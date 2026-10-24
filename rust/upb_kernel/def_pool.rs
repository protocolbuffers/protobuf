// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::upb_reflection::{self, DefPool, DefPoolInitPtr, MessageDef};
use super::{
    MiniTableEnumPtr, MiniTableExtensionInitPtr, MiniTableExtensionPtr, MiniTablePtr,
    EXTENSION_MINI_TABLE_TO_DEF_INITS, THREAD_LOCAL_ARENA,
};
use std::collections::HashMap;
use std::ffi::CStr;
use std::sync::{Mutex, MutexGuard, OnceLock};

#[derive(Clone, Copy)]
pub struct DefPoolInit(DefPoolInitPtr);
unsafe impl Send for DefPoolInit {}
unsafe impl Sync for DefPoolInit {}

/// Holds the MessageDef of one message once `message_def` has looked it up.
#[derive(Default)]
pub struct MessageDefCached(OnceLock<MessageDef<'static>>);
unsafe impl Sync for MessageDefCached {}

impl MessageDefCached {
    pub const fn new() -> Self {
        Self(OnceLock::new())
    }
}

pub(super) struct GlobalDefPool(pub(super) DefPool);
unsafe impl Send for GlobalDefPool {}
// There is only one global, singleton DefPool, protected by a Mutex.
static POOL: OnceLock<Mutex<GlobalDefPool>> = OnceLock::new();

// Maps each extension's MiniTable pointer to the `def_init` function of the file that declares it.
type ExtMiniTableToDefInit = HashMap<MiniTableExtensionInitPtr, fn() -> DefPoolInit>;

static EXT_MT_TO_DEF_INIT: OnceLock<ExtMiniTableToDefInit> = OnceLock::new();

/// Returns a global, static map of each extension's MiniTable pointer to the `def_init` function of
/// the file that declares it. If the map does not yet exist, creates it.
fn ext_mt_to_def_init() -> &'static ExtMiniTableToDefInit {
    EXT_MT_TO_DEF_INIT.get_or_init(|| {
        let mut map = ExtMiniTableToDefInit::new();
        for ext in EXTENSION_MINI_TABLE_TO_DEF_INITS {
            map.insert(**ext.mini_table, ext.def_init);
        }
        map
    })
}

// A C-compatible fallback hook to resolve missing extension inits by looking them up in the
// `ext_mt_to_def_init` map.
extern "C" fn resolve_ext_def_init(ext: MiniTableExtensionPtr) -> Option<DefPoolInitPtr> {
    let def_init = ext_mt_to_def_init().get(&MiniTableExtensionInitPtr(ext))?;
    Some(def_init().0)
}

// Returns a lock guard of the global DefPool. If the pool does not yet exist, creates it.
pub(super) fn global_def_pool() -> MutexGuard<'static, GlobalDefPool> {
    POOL.get_or_init(|| {
        let mut pool = DefPool::new();
        // SAFETY:
        // - `resolve_ext_def_init` returns either `None` or the generated init of the file that
        //   declares the extension, whose layout contains that extension's MiniTable.
        // - Generated inits, their deps and their layouts are never freed.
        // - `resolve_ext_def_init` never accesses the pool.
        unsafe { pool.set_ext_init_resolver(resolve_ext_def_init) };
        Mutex::new(GlobalDefPool(pool))
    })
    .lock()
    .expect("global DefPool mutex should not be poisoned")
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
    *(T::message_def_cached().0.get_or_init(|| {
        // Eventually calls `build_def_init` emitted for `T`.
        let init = T::def_init();

        let mut pool = global_def_pool();

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
