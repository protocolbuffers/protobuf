// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::*;
use crate::__internal::entity_tag::*;
use crate::__internal::{EntityType, Enum, Private, Singular};
use crate::extension::{ExtAccess, ExtClear, ExtGetMut, ExtHas};
use crate::{ExtensionId, IntoMut, IntoView, MessageViewInterop, Proxied};
use std::sync::LazyLock;

#[cold]
#[inline(never)]
#[track_caller]
fn set_extension_failed() -> ! {
    panic!("upb_Message_SetExtension failed (alloc should be infallible)");
}

pub struct InnerExtensionId {
    mini_table: &'static LazyLock<MiniTableExtensionInitPtr>,
}

impl InnerExtensionId {
    pub const fn new(mini_table: &'static LazyLock<MiniTableExtensionInitPtr>) -> Self {
        Self { mini_table }
    }

    pub fn mini_table(&self) -> RawMiniTableExtension {
        self.mini_table.0
    }
}

impl<Extendee, T: Proxied> ExtensionId<Extendee, T> {
    /// The `upb_MiniTableExtension` this extension was built with.
    ///
    /// For generated code only. Forces the MiniTable to be built if it hasn't been yet; once
    /// built it lives for the rest of the process, like every other MiniTable in the kernel.
    #[doc(hidden)]
    pub fn __internal_mini_table(&self, _private: Private) -> RawMiniTableExtension {
        self.inner.mini_table()
    }
}

#[cfg(not(target_os = "emscripten"))]
#[linkme::distributed_slice]
pub static EXTENSIONS: [LazyLock<MiniTableExtensionInitPtr>];

// The sentinel sections use ELF-only directives, so skip every Apple (Mach-O) target, not just
// macOS. Emscripten has no linkme backend (see below).
#[cfg(all(target_family = "unix", not(target_vendor = "apple"), not(target_os = "emscripten")))]
core::arch::global_asm!(
    r#"
    .section linkme_EXTENSIONS,"awR",@progbits
    .globl LINKME_SENTINEL_EXTENSIONS
    LINKME_SENTINEL_EXTENSIONS:
    .size   LINKME_SENTINEL_EXTENSIONS, 0

    .section linkm2_EXTENSIONS,"awR",@progbits
    .globl LINKM2_SENTINEL_EXTENSIONS
    LINKM2_SENTINEL_EXTENSIONS:
    .size   LINKM2_SENTINEL_EXTENSIONS, 0
    "#
);

// Add the extension's MiniTable and the def_init of the file that declares it to the global
// EXTENSION_MINI_TABLE_TO_DEF_INITS registry; this is used for reflection.
//
// Registered the same way as EXTENSIONS.
pub struct ExtensionMiniTableToDefInit {
    pub mini_table: &'static LazyLock<MiniTableExtensionInitPtr>,
    pub def_init: fn() -> DefPoolInit,
}

#[cfg(not(target_os = "emscripten"))]
#[linkme::distributed_slice]
pub static EXTENSION_MINI_TABLE_TO_DEF_INITS: [ExtensionMiniTableToDefInit];

#[cfg(all(target_family = "unix", not(target_vendor = "apple"), not(target_os = "emscripten")))]
core::arch::global_asm!(
    r#"
    .section linkme_EXTENSION_MINI_TABLE_TO_DEF_INITS,"awR",@progbits
    .globl LINKME_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS
    LINKME_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS:
    .size   LINKME_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS, 0

    .section linkm2_EXTENSION_MINI_TABLE_TO_DEF_INITS,"awR",@progbits
    .globl LINKM2_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS
    LINKM2_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS:
    .size   LINKM2_SENTINEL_EXTENSION_MINI_TABLE_TO_DEF_INITS, 0
    "#
);

// wasm32-unknown-emscripten: `linkme` has no backend for this target ("distributed_slice is not
// implemented for this platform"), and it can't have one: rustc places `#[link_section]` statics
// in Wasm *custom sections* (not addressable memory), except for `.init_array`. So on Emscripten
// each generated extension registers itself from a static constructor instead: the
// `EXTENSIONS!` macro below accepts the same element syntax that `#[linkme::distributed_slice]`
// expands to in generated code, and emits a `.init_array` entry that runs from
// `__wasm_call_ctors` (i.e. `_initialize`) before any parse. `EXTENSIONS` (the value) stays an
// empty `DistributedSlice` so generated code still type-checks.
//
// Caveats: (1) rustc emits no `symbols.o` for Wasm, so wasm-ld only links a constructor if its
// object file is pulled in by some other referenced symbol; an extension in a crate (codegen
// unit) that nothing references is silently not registered (native linkme keeps it). (2) The
// constructors use priority 101 so they run before default-priority (65535) static initializers;
// a parse from an earlier constructor would freeze the registry without them. (3) On
// `wasm32-unknown-emscripten`, `#[used]` on `__PROTOBUF_EXTENSION_CTOR` causes LLVM's Wasm
// backend to mark the static `WASM_SYMBOL_EXPORTED`, so `wasm-ld`/`emcc` emits a Wasm global and
// export entry per registered extension.
#[cfg(target_os = "emscripten")]
static EXTENSIONS_EMPTY: [LazyLock<MiniTableExtensionInitPtr>; 0] = [];

// Verified against `linkme` 0.3.35 (`third_party/rust/linkme/v0_3`, pinned as `linkme = "=0.3.35"`
// in `rust/release_crates/google_protobuf/Cargo-template.toml`): relies on `#[doc(hidden)]
// DistributedSlice::private_new` and `linkme_impl` 0.3.35's `#![linkme_macro = ...]` /
// `#![linkme_sort_key = ...]` expansion grammar in `__protobuf_upb_emscripten_extensions` below.
#[cfg(target_os = "emscripten")]
pub static EXTENSIONS: linkme::DistributedSlice<[LazyLock<MiniTableExtensionInitPtr>]> = unsafe {
    linkme::DistributedSlice::private_new(
        "EXTENSIONS",
        core::ptr::addr_of!(EXTENSIONS_EMPTY),
        core::ptr::addr_of!(EXTENSIONS_EMPTY),
        core::ptr::addr_of!(EXTENSIONS_EMPTY).cast::<()>(),
        core::ptr::addr_of!(EXTENSIONS_EMPTY).cast::<()>(),
    )
};

#[cfg(target_os = "emscripten")]
static EMSCRIPTEN_EXTENSIONS: std::sync::Mutex<Vec<&'static LazyLock<MiniTableExtensionInitPtr>>> =
    std::sync::Mutex::new(Vec::new());

/// For generated code only (via `EXTENSIONS!`): records one extension for the registry.
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_register_extension(ext: &'static LazyLock<MiniTableExtensionInitPtr>) {
    debug_assert!(
        !EMSCRIPTEN_REGISTRY_BUILT.load(std::sync::atomic::Ordering::Relaxed),
        "protobuf extension registered after the extension registry was built"
    );
    EMSCRIPTEN_EXTENSIONS.lock().unwrap_or_else(|e| e.into_inner()).push(ext);
}

#[cfg(target_os = "emscripten")]
static EMSCRIPTEN_REGISTRY_BUILT: std::sync::atomic::AtomicBool =
    std::sync::atomic::AtomicBool::new(false);

/// Number of extensions registered by static constructors (diagnostics for Wasm probes).
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_registered_extension_count() -> usize {
    EMSCRIPTEN_EXTENSIONS.lock().unwrap_or_else(|e| e.into_inner()).len()
}

#[cfg(target_os = "emscripten")]
#[doc(hidden)]
#[macro_export]
macro_rules! __protobuf_upb_emscripten_extensions {
    (
        #![linkme_macro = $macro:path]
        $(#![linkme_sort_key = $key:tt])*
        $(#[$attr:meta])*
        $vis:vis static $name:ident : $ty:ty = $init:expr;
    ) => {
        $(#[$attr])*
        $vis static $name: $ty = $init;
        const _: () = {
            extern "C" fn __protobuf_register_extension() {
                $crate::__internal::runtime::__emscripten_register_extension(&$name);
            }
            #[used]
            #[unsafe(link_section = ".init_array.101")]
            static __PROTOBUF_EXTENSION_CTOR: extern "C" fn() = __protobuf_register_extension;
        };
    };
}

#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub use __protobuf_upb_emscripten_extensions as EXTENSIONS;

// Same Emscripten scheme for EXTENSION_MINI_TABLE_TO_DEF_INITS (reflection): generated code's
// `#[linkme::distributed_slice($pbr$::EXTENSION_MINI_TABLE_TO_DEF_INITS)]` expands to the
// `EXTENSION_MINI_TABLE_TO_DEF_INITS!` macro below, which registers the element from a
// `.init_array` constructor; `load_all_extensions` (def_pool.rs) reads them via
// `__emscripten_extension_mini_table_to_def_inits`. Both registrations of one extension are
// emitted in the same generated block, so they live in the same object file and are linked (or
// dropped, see caveat (1) above) together. With reflection support each extension therefore
// exports two constructor globals (`__PROTOBUF_EXTENSION_CTOR` and
// `__PROTOBUF_EXTENSION_DEF_INIT_CTOR`, see caveat (3) above). Lite-runtime builds (the default
// Wasm toolchain implies `proto_force_lite_runtime`) emit no def-init elements.
#[cfg(target_os = "emscripten")]
static EXTENSION_MINI_TABLE_TO_DEF_INITS_EMPTY: [ExtensionMiniTableToDefInit; 0] = [];

#[cfg(target_os = "emscripten")]
pub static EXTENSION_MINI_TABLE_TO_DEF_INITS: linkme::DistributedSlice<
    [ExtensionMiniTableToDefInit],
> = unsafe {
    linkme::DistributedSlice::private_new(
        "EXTENSION_MINI_TABLE_TO_DEF_INITS",
        core::ptr::addr_of!(EXTENSION_MINI_TABLE_TO_DEF_INITS_EMPTY),
        core::ptr::addr_of!(EXTENSION_MINI_TABLE_TO_DEF_INITS_EMPTY),
        core::ptr::addr_of!(EXTENSION_MINI_TABLE_TO_DEF_INITS_EMPTY).cast::<()>(),
        core::ptr::addr_of!(EXTENSION_MINI_TABLE_TO_DEF_INITS_EMPTY).cast::<()>(),
    )
};

#[cfg(target_os = "emscripten")]
static EMSCRIPTEN_EXTENSION_DEF_INITS: std::sync::Mutex<Vec<&'static ExtensionMiniTableToDefInit>> =
    std::sync::Mutex::new(Vec::new());

/// For generated code only (via `EXTENSION_MINI_TABLE_TO_DEF_INITS!`).
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_register_extension_def_init(entry: &'static ExtensionMiniTableToDefInit) {
    debug_assert!(
        !EMSCRIPTEN_DEF_INITS_LOADED.load(std::sync::atomic::Ordering::Relaxed),
        "protobuf extension def_init registered after load_all_extensions ran"
    );
    EMSCRIPTEN_EXTENSION_DEF_INITS.lock().unwrap_or_else(|e| e.into_inner()).push(entry);
}

/// Set by `load_all_extensions` (def_pool.rs) when it reads the registrations; a later
/// registration (e.g. reflection used from an earlier static constructor) would be dropped.
#[cfg(target_os = "emscripten")]
static EMSCRIPTEN_DEF_INITS_LOADED: std::sync::atomic::AtomicBool =
    std::sync::atomic::AtomicBool::new(false);

/// Called by `load_all_extensions` before it reads the registrations.
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_mark_extension_def_inits_loaded() {
    EMSCRIPTEN_DEF_INITS_LOADED.store(true, std::sync::atomic::Ordering::Relaxed);
}

/// Number of extension def_inits registered by static constructors (diagnostics for Wasm probes).
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_registered_extension_def_init_count() -> usize {
    EMSCRIPTEN_EXTENSION_DEF_INITS.lock().unwrap_or_else(|e| e.into_inner()).len()
}

/// Entries registered by static constructors (read by `load_all_extensions`).
#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub fn __emscripten_extension_mini_table_to_def_inits() -> Vec<&'static ExtensionMiniTableToDefInit>
{
    EMSCRIPTEN_EXTENSION_DEF_INITS.lock().unwrap_or_else(|e| e.into_inner()).clone()
}

#[cfg(target_os = "emscripten")]
#[doc(hidden)]
#[macro_export]
macro_rules! __protobuf_upb_emscripten_extension_def_inits {
    (
        #![linkme_macro = $macro:path]
        $(#![linkme_sort_key = $key:tt])*
        $(#[$attr:meta])*
        $vis:vis static $name:ident : $ty:ty = $init:expr;
    ) => {
        $(#[$attr])*
        $vis static $name: $ty = $init;
        const _: () = {
            extern "C" fn __protobuf_register_extension_def_init() {
                $crate::__internal::runtime::__emscripten_register_extension_def_init(&$name);
            }
            #[used]
            #[unsafe(link_section = ".init_array.101")]
            static __PROTOBUF_EXTENSION_DEF_INIT_CTOR: extern "C" fn() =
                __protobuf_register_extension_def_init;
        };
    };
}

#[cfg(target_os = "emscripten")]
#[doc(hidden)]
pub use __protobuf_upb_emscripten_extension_def_inits as EXTENSION_MINI_TABLE_TO_DEF_INITS;

pub fn generated_extension_registry() -> RawExtensionRegistry {
    static EXTENSIONS_REGISTRY: LazyLock<ExtensionRegistryInitPtr> = LazyLock::new(|| unsafe {
        let registry = upb_ExtensionRegistry_New(THREAD_LOCAL_ARENA.with(|a| a.raw()));
        for extension in EXTENSIONS {
            upb_ExtensionRegistry_Add(registry, extension.0);
        }
        #[cfg(target_os = "emscripten")]
        {
            EMSCRIPTEN_REGISTRY_BUILT.store(true, std::sync::atomic::Ordering::Relaxed);
            for extension in EMSCRIPTEN_EXTENSIONS.lock().unwrap_or_else(|e| e.into_inner()).iter()
            {
                upb_ExtensionRegistry_Add(registry, extension.0);
            }
        }
        ExtensionRegistryInitPtr(registry)
    });
    EXTENSIONS_REGISTRY.0
}

impl<Msg: Message, T: Singular> ExtHas<Msg> for ExtensionId<Msg, T> {
    fn has(&self, _private: Private, msg: impl AsView<Proxied = Msg>) -> bool {
        // SAFETY:
        // - `msg` is a valid message.
        // - `mini_table` is the one associated with `self`.
        unsafe {
            upb_Message_HasExtension(
                msg.as_view().get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        }
    }
}

impl<Extendee: Message, V: Message> ExtAccess<Extendee, V, MessageTag>
    for ExtensionId<Extendee, V>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, V> {
        let msg = msg.into_view();
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            upb_Message_GetExtensionMessage(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                default_instance.get_ptr(Private).raw(),
            )
        };
        unsafe {
            <View<'msg, V>>::__unstable_wrap_raw_message_unchecked_lifetime(
                raw_msg.as_ptr() as *const std::ffi::c_void
            )
        }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<V>,
    ) {
        let mut msg_mut = msg.as_mut();

        let mut child = value.into_proxied(Private);
        let child_ptr = child.get_ptr_mut(Private);

        // Ensure child arena is fused into parent message
        msg_mut.get_arena(Private).fuse(child.get_arena(Private));

        unsafe {
            if !upb_Message_SetExtensionMessage(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                child_ptr.raw(),
                msg_mut.get_arena(Private).raw(),
            ) {
                set_extension_failed();
            }
        }
    }
}

impl<Extendee: Message, V: Message> ExtGetMut<Extendee, V, MessageTag>
    for ExtensionId<Extendee, V>
{
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Extendee>,
    ) -> Mut<'msg, V> {
        let mut msg = msg.into_mut();
        unsafe {
            // SAFETY: The arena associated with a `Mut<'msg, _>` proxy lives for at least `'msg`.
            // `UpbGetArena::get_arena` returns an `&Arena` tied to the local borrow of `msg`,
            // but we can safely extend it back to `'msg` here because `msg` holds an `&'msg Arena`.
            let arena_ref: &'msg Arena = &*(msg.get_arena(Private) as *const Arena);

            // TODO: upb should have a GetOrCreateExtension operation instead of this dance.
            let raw_msg = match upb_Message_HasExtension(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            ) {
                true => {
                    upb_Message_GetExtensionMessage(
                        msg.get_ptr_mut(Private).raw(),
                        self.inner.mini_table().as_ptr(),
                        NonNull::dangling(), // Not used.
                    )
                }
                false => {
                    let raw_msg =
                        MessagePtr::<V>::new(arena_ref).expect("alloc should never fail").raw();
                    upb_Message_SetExtensionMessage(
                        msg.get_ptr_mut(Private).raw(),
                        self.inner.mini_table().as_ptr(),
                        raw_msg,
                        arena_ref.raw(),
                    );
                    raw_msg
                }
            };
            V::from_message_mut(raw_msg, arena_ref)
        }
    }
}

impl<Extendee: Message, V: Singular> ExtAccess<Extendee, Repeated<V>, RepeatedTag>
    for ExtensionId<Extendee, Repeated<V>>
where
    V: EntityType + UpbTypeConversions<V::Tag>,
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, Repeated<V>> {
        let msg = msg.into_view();
        let raw_array = unsafe {
            upb_Message_GetExtensionArray(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        };
        unsafe {
            IntoView::into_view(
                raw_array.map_or_else(empty_array::<V>, |raw| RepeatedView::from_raw(Private, raw)),
            )
        }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<Repeated<V>>,
    ) {
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.clear();
        ext_mut.copy_from(unsafe { RepeatedView::from_raw(Private, value.inner(Private).raw()) });
    }
}

impl<Extendee: Message, V> ExtGetMut<Extendee, Repeated<V>, RepeatedTag>
    for ExtensionId<Extendee, Repeated<V>>
where
    V: Singular + EntityType + UpbTypeConversions<V::Tag>,
{
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Extendee>,
    ) -> Mut<'msg, Repeated<V>> {
        let mut msg = msg.into_mut();

        // SAFETY: The arena associated with a `Mut<'msg, _>` proxy lives for at least `'msg`.
        // `UpbGetArena::get_arena` returns an `&Arena` tied to the local borrow of `msg`,
        // but we can safely extend it back to `'msg` here because `msg` holds an `&'msg Arena`.
        let arena_ref: &'msg Arena = unsafe { &*(msg.get_arena(Private) as *const Arena) };
        let raw_array = unsafe {
            upb_Message_GetExtensionMutableArray(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        };
        let raw_array = raw_array.unwrap_or_else(|| unsafe {
            let new_arr = upb_Array_New(arena_ref.raw(), V::upb_type());
            let mut ptr = new_arr.as_ptr();
            upb_Message_SetExtension(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                &mut ptr as *mut _ as *const core::ffi::c_void,
                arena_ref.raw(),
            );
            new_arr
        });
        unsafe {
            RepeatedMut::from_inner(Private, InnerRepeatedMut::new(raw_array, arena_ref)).into_mut()
        }
    }
}

macro_rules! impl_upb_scalar_extension {
    ($($t:ty => [$get:ident, $set:ident]),* $(,)?) => {
        $(
            impl<Extendee: Message> ExtAccess<Extendee, $t, PrimitiveTag> for ExtensionId<Extendee, $t> {
                fn get<'msg>(&self, _private: Private, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, $t> {
                    let msg = msg.into_view();
                    unsafe {
                        $get(
                            msg.get_ptr(Private).raw(),
                            self.inner.mini_table().as_ptr(),
                            self.default.expect("scalar extensions must have a default value"),
                        )
                    }
                }

                fn set(&self, _private: Private, mut msg: impl AsMut<MutProxied = Extendee>, value: impl IntoProxied<$t>) {
                    let mut msg_mut = msg.as_mut();
                    unsafe {
                        if !$set(
                            msg_mut.get_ptr_mut(Private).raw(),
                            self.inner.mini_table().as_ptr(),
                            value.into_proxied(Private),
                            msg_mut.get_arena(Private).raw(),
                        ) {
                            set_extension_failed();
                        }
                    }
                }
            }
        )*
    };
}

impl_upb_scalar_extension!(
    bool => [upb_Message_GetExtensionBool, upb_Message_SetExtensionBool],
    f32 => [upb_Message_GetExtensionFloat, upb_Message_SetExtensionFloat],
    f64 => [upb_Message_GetExtensionDouble, upb_Message_SetExtensionDouble],
    i32 => [upb_Message_GetExtensionInt32, upb_Message_SetExtensionInt32],
    i64 => [upb_Message_GetExtensionInt64, upb_Message_SetExtensionInt64],
    u32 => [upb_Message_GetExtensionUInt32, upb_Message_SetExtensionUInt32],
    u64 => [upb_Message_GetExtensionUInt64, upb_Message_SetExtensionUInt64],
);

impl<Extendee: Message> ExtAccess<Extendee, ProtoString, PrimitiveTag>
    for ExtensionId<Extendee, ProtoString>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, ProtoString> {
        let msg = msg.into_view();
        let upb_str_view = unsafe {
            upb_Message_GetExtensionString(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                self.default.expect("string extensions must have a default value").into(),
            )
        };
        unsafe { ProtoStr::from_utf8_unchecked(upb_str_view.as_ref()) }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoString>,
    ) {
        let s = value.into_proxied(Private);
        let (view, arena) = s.into_inner(Private).into_raw_parts();
        let mut msg_mut = msg.as_mut();
        msg_mut.get_arena(Private).fuse(&arena);
        unsafe {
            if !upb_Message_SetExtensionString(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                view,
                msg_mut.get_arena(Private).raw(),
            ) {
                set_extension_failed();
            }
        }
    }
}

impl<Extendee: Message> ExtAccess<Extendee, ProtoBytes, PrimitiveTag>
    for ExtensionId<Extendee, ProtoBytes>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, ProtoBytes> {
        let msg = msg.into_view();
        let upb_str_view = unsafe {
            upb_Message_GetExtensionString(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                self.default.expect("bytes extensions must have a default value").into(),
            )
        };
        unsafe { upb_str_view.as_ref() }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoBytes>,
    ) {
        let s = value.into_proxied(Private);
        let (view, arena) = s.into_inner(Private).into_raw_parts();
        let mut msg_mut = msg.as_mut();
        msg_mut.get_arena(Private).fuse(&arena);
        unsafe {
            if !upb_Message_SetExtensionString(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                view,
                msg_mut.get_arena(Private).raw(),
            ) {
                set_extension_failed();
            }
        }
    }
}

impl<Msg: Message, T: Proxied> ExtClear<Msg> for ExtensionId<Msg, T> {
    fn clear(&self, _private: Private, mut msg: impl AsMut<MutProxied = Msg>) {
        // SAFETY:
        // - `msg` is a valid message.
        // - `mini_table` is the one associated with `self`.
        unsafe {
            upb_Message_ClearExtension(
                msg.as_mut().get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        }
    }
}

impl<Extendee: Message, E> ExtAccess<Extendee, E, EnumTag> for ExtensionId<Extendee, E>
where
    E: Enum + Proxied + EntityType<Tag = EnumTag> + Copy,
    for<'a> E: Proxied<View<'a> = E>,
    for<'a> Extendee::MessageView<'a>: UpbGetMessagePtr,
    i32: From<E>,
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, E> {
        let msg = msg.into_view();
        let default = self.default.expect("enum extensions must have a default value");
        let val = unsafe {
            upb_Message_GetExtensionInt32(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                i32::from(default),
            )
        };
        E::try_from(val).unwrap_or(default)
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<E>,
    ) {
        let mut msg_mut = msg.as_mut();
        unsafe {
            if !upb_Message_SetExtensionInt32(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                value.into_proxied(Private).into(),
                msg_mut.get_arena(Private).raw(),
            ) {
                set_extension_failed();
            }
        }
    }
}
