// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::upb_reflection::{self, DefPool, DefPoolInitPtr, MessageDef};
use super::{
    MiniTableEnumPtr, MiniTableExtensionPtr, MiniTablePtr, EXTENSION_MINI_TABLE_TO_DEF_INITS,
    THREAD_LOCAL_ARENA,
};
use std::collections::HashSet;
use std::ffi::CStr;
use std::sync::{Once, OnceLock, RwLock, RwLockReadGuard, RwLockWriteGuard};

#[derive(Clone, Copy)]
pub struct DefPoolInit(DefPoolInitPtr);
unsafe impl Send for DefPoolInit {}
unsafe impl Sync for DefPoolInit {}

/// Holds what is computed around a message, once it has been looked up.
#[derive(Default)]
pub struct MessageDefCached {
    // The message's MessageDef.
    def: OnceLock<MessageDef<'static>>,
    // Whether the message may contain extensions.
    may_contain_extensions: OnceLock<bool>,
}
unsafe impl Sync for MessageDefCached {}

impl MessageDefCached {
    pub const fn new() -> Self {
        Self { def: OnceLock::new(), may_contain_extensions: OnceLock::new() }
    }
}

pub(super) struct GlobalDefPool(pub(super) DefPool);
unsafe impl Send for GlobalDefPool {}
unsafe impl Sync for GlobalDefPool {}

// There is only one global, singleton DefPool, protected by a RwLock.
static POOL: OnceLock<RwLock<GlobalDefPool>> = OnceLock::new();

fn global_def_pool() -> &'static RwLock<GlobalDefPool> {
    POOL.get_or_init(|| RwLock::new(GlobalDefPool(DefPool::new())))
}

// Returns a read lock on the global DefPool.
pub(super) fn read_global_def_pool() -> RwLockReadGuard<'static, GlobalDefPool> {
    global_def_pool().read().expect("global DefPool lock should not be poisoned")
}

// Returns a write lock on the global DefPool.
fn write_global_def_pool() -> RwLockWriteGuard<'static, GlobalDefPool> {
    global_def_pool().write().expect("global DefPool lock should not be poisoned")
}

/// A message of which a MessageDef and DefPoolInit can be looked up. This trait must be
/// implemented in Rust gencode for every message that needs reflection support.
///
/// # Safety
/// - `FULL_NAME` must be the message's fully-qualified name.
/// - `def_init()` must return the init of the .proto file that declares the message.
/// - `message_def_cached()` must return a static MessageDef.
pub unsafe trait UpbWithReflection {
    const FULL_NAME: &'static str;

    fn def_init() -> DefPoolInit;

    // For each message type, there is only one static, cached pointer to its MessageDef.
    fn message_def_cached() -> &'static MessageDefCached;
}

/// Builds a upb_DefPool_Init
///
/// # Safety
/// - `filename` and `descriptor` must be the file's name and its serialized `FileDescriptorProto`.
/// - `deps` must be the inits of the files it imports.
/// - `msgs`, `enums` and `exts` must be the file's own MiniTables, in the order upb's generator
///   lists them in a `upb_MiniTableFile` (see `SortedMessages`, `SortedEnums` and
///   `SortedExtensions` in upb_generator/file_layout.h).
pub unsafe fn build_def_init(
    filename: &'static CStr,
    descriptor: &'static [u8],
    deps: &[DefPoolInit],
    msgs: &[MiniTablePtr],
    enums: &[MiniTableEnumPtr],
    exts: &[MiniTableExtensionPtr],
) -> DefPoolInit {
    let mut raw_deps: Vec<DefPoolInitPtr> = Vec::with_capacity(deps.len());
    for dep in deps {
        raw_deps.push(dep.0);
    }

    THREAD_LOCAL_ARENA.with(|arena| unsafe {
        let layout = upb_reflection::build_mini_table_file(arena, msgs, enums, exts);
        DefPoolInit(upb_reflection::build_def_pool_init(
            arena, filename, descriptor, &raw_deps, layout,
        ))
    })
}

/// Returns the MessageDef of T.
pub fn message_def<T: UpbWithReflection>() -> MessageDef<'static> {
    *(T::message_def_cached().def.get_or_init(|| {
        // Eventually calls `build_def_init` emitted for `T`.
        let init = T::def_init();

        let mut pool = write_global_def_pool();

        // Load the descriptor into the global DefPool. If it was already loaded, this would be a
        // no-op.
        //
        // SAFETY: `init` was only built for the file that declares `T`.
        let loaded = unsafe { pool.0.load_def_init(init.0.as_ptr()) };
        assert!(loaded, "failed to load a generated descriptor into the global DefPool");

        let def = pool
            .0
            .find_message_by_name(T::FULL_NAME)
            .expect("a generated message is missing from its own file's descriptor");

        // SAFETY: `def` was allocated by the global pool, which is never freed.
        unsafe { MessageDef::from_raw(def.raw()) }
    }))
}

/// Returns whether a message of type T may contain extensions. The result is cached.
///
/// "contain" here means that the message is either itself extendable, or it can reach a submessage
/// that is extendable. Whether the message is actually extended is not required for this to
/// be true.
pub fn may_contain_extensions<T: UpbWithReflection>() -> bool {
    *T::message_def_cached()
        .may_contain_extensions
        .get_or_init(|| reaches_extendable_message_from_message_def(message_def::<T>()))
}

// Returns whether `msg_def`, or any message type reachable from its fields, declares an extension
// range.
fn reaches_extendable_message_from_message_def(msg_def: MessageDef<'_>) -> bool {
    let mut visited = HashSet::from([msg_def.raw()]);
    let mut stack = vec![msg_def];
    // Perform a non-recursive DFS in case of deeply-nested schemas.
    while let Some(msg_def) = stack.pop() {
        // If the message declares an extension range, then it **could** be extended.
        if msg_def.extension_range_count() > 0 {
            return true;
        }
        for i in 0..msg_def.field_count() {
            let Some(sub_msg_def) = msg_def.field(i).message_sub_def() else {
                continue;
            };
            if visited.insert(sub_msg_def.raw()) {
                stack.push(sub_msg_def);
            }
        }
    }
    false
}

/// Loads the file of every extension linked into the binary into the global DefPool.
pub(super) fn load_all_extensions() {
    static LOADED: Once = Once::new();
    LOADED.call_once(|| {
        // Like `message_def`, build the inits before taking the lock.
        let inits: Vec<DefPoolInit> =
            EXTENSION_MINI_TABLE_TO_DEF_INITS.iter().map(|ext| (ext.def_init)()).collect();

        let mut pool = write_global_def_pool();
        for init in inits {
            // Many extensions share a file, which is only loaded the first time; loading it again
            // is a no-op.
            //
            // SAFETY: `init` was only built for the file that declares the extension.
            let loaded = unsafe { pool.0.load_def_init(init.0.as_ptr()) };
            assert!(loaded, "failed to load a generated descriptor into the global DefPool");
        }
    });
}
