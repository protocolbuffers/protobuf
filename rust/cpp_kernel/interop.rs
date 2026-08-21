// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits related to interop with the underlying C++ Proto types.
//!
//! These traits are deliberately not available on the prelude, as they should
//! be used rarely and with great care.

use super::*;

/// Methods for converting to and from a raw, owned C++ message pointer.
pub trait OwnedMessageInterop: SealedInternal {
    /// Drops `self` and returns an underlying pointer that it was wrapping
    /// without deleting it.
    ///
    /// The caller is responsible for ensuring the returned pointer is
    /// subsequently deleted (eg by moving it into a std::unique_ptr in
    /// C++), or else it will leak.
    fn __unstable_leak_raw_message(self) -> *mut std::ffi::c_void;

    /// Takes exclusive ownership of the `raw_message`.
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The pointer passed in must not be used by the caller after being
    ///     passed here (must not be read, written, or deleted)
    unsafe fn __unstable_take_ownership_of_raw_message(raw_message: *mut std::ffi::c_void) -> Self;
}

/// Methods for converting to and from a raw C++ message view pointer.
pub trait MessageViewInterop<'msg>: SealedInternal {
    /// Borrows `self` as an underlying C++ raw pointer.
    ///
    /// Note that the returned Value must be used under the same constraints
    /// as though it were a borrow of `self`: it should be treated as a
    /// `const Message*` in C++, and not be mutated in any way, and any
    /// mutation to the parent message may invalidate it, and it
    /// must not be deleted.
    fn __unstable_as_raw_message(&self) -> *const std::ffi::c_void;

    /// Wraps the provided pointer as a MessageView.
    ///
    /// This takes a ref of a pointer so that a stack variable's lifetime
    /// can be used for a safe lifetime; under most cases this is
    /// the correct lifetime and this should be used as:
    /// ```ignore
    /// fn called_from_cpp(msg: *const c_void) {
    ///   // `msg` is known live for the current stack frame, so view's
    ///   // lifetime is also tied to the current stack frame here:
    ///   let view = unsafe { __unstable_wrap_raw_message(&msg) };
    ///   do_something_with_view(view);
    /// }
    /// ```
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The underlying message must be alive for 'msg and not mutated
    ///     while the wrapper is live.
    unsafe fn __unstable_wrap_raw_message(raw: &'msg *const std::ffi::c_void) -> Self;

    /// Wraps the provided pointer as a MessageView.
    ///
    /// Unlike `__unstable_wrap_raw_message` this has no constraints
    /// on lifetime: the caller has a free choice for the lifetime.
    ///
    /// As this is much easier to get the lifetime wrong than
    /// `__unstable_wrap_raw_message`, prefer using that wherever
    /// your lifetime can be tied to a stack lifetime, and only use this one
    /// if its not possible (e.g. with a 'static lifetime).
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The underlying message must be alive for the caller-chosen 'msg
    ///     and not mutated while the wrapper is live.
    unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(raw: *const std::ffi::c_void) -> Self;
}

/// Methods for converting to and from a raw, mutable C++ message pointer.
pub trait MessageMutInterop<'msg>: SealedInternal {
    /// Exclusive borrows `self` as an underlying mutable C++ raw pointer.
    ///
    /// Note that the returned Value must be used under the same constraints
    /// as though it were a mut borrow of `self`: it should be treated as a
    /// non-owned `Message*` in C++. And any mutation to the parent message
    /// may invalidate it, and it must not be deleted.
    fn __unstable_as_raw_message_mut(&mut self) -> *mut std::ffi::c_void;

    /// Wraps the provided C++ pointer as a MessageMut.
    ///
    /// This takes a ref of a pointer so that a stack variable's lifetime
    /// can be used for a safe lifetime; under most cases this is
    /// the correct lifetime and this should be used as:
    /// ```ignore
    /// fn called_from_cpp(msg: *mut c_void) {
    ///   // `msg` is known live for the current stack frame, so mut's
    ///   // lifetime is also tied to the current stack frame here:
    ///   let m = unsafe { __unstable_wrap_raw_message_mut(&mut msg) };
    ///   do_something_with_mut(m);
    /// }
    /// ```
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The underlying message must be alive for 'msg and not read or
    ///     mutated while the wrapper is live.
    unsafe fn __unstable_wrap_raw_message_mut(raw: &'msg mut *mut std::ffi::c_void) -> Self;

    /// Wraps the provided pointer as a MessageMut.
    ///
    /// Unlike `__unstable_wrap_raw_message_mut` this has no constraints
    /// on lifetime: the caller has a free choice for the lifetime.
    ///
    /// As this is much easier to get the lifetime wrong than
    /// `__unstable_wrap_raw_message_mut`, prefer using that wherever
    /// the lifetime can be tied to a stack lifetime, and only use this one
    /// if its not possible (e.g. with a 'static lifetime).
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The underlying message must be alive for the caller-chosen 'msg
    ///     and not mutated while the wrapper is live.
    unsafe fn __unstable_wrap_raw_message_mut_unchecked_lifetime(
        raw: *mut std::ffi::c_void,
    ) -> Self;
}

/// Note that this is only implemented for the types implementing `proto2::Message`.
#[cfg(not(lite_runtime))]
pub trait MessageDescriptorInterop {
    /// Returns a pointer to a `proto2::Descriptor` or `nullptr` if the
    /// descriptor is not available.
    fn __unstable_get_descriptor() -> *const std::ffi::c_void;
}

impl<'a, T> MessageMutInterop<'a> for T
where
    Self: AsMut + CppGetRawMessageMut + From<MessageMutInner<'a, <Self as AsMut>::MutProxied>>,
    <Self as AsMut>::MutProxied: Message,
{
    unsafe fn __unstable_wrap_raw_message_mut(msg: &'a mut *mut std::ffi::c_void) -> Self {
        let raw = RawMessage::new(*msg as *mut _).unwrap();
        let inner = unsafe { MessageMutInner::wrap_raw(raw) };
        inner.into()
    }
    unsafe fn __unstable_wrap_raw_message_mut_unchecked_lifetime(
        msg: *mut std::ffi::c_void,
    ) -> Self {
        let raw = RawMessage::new(msg as *mut _).unwrap();
        let inner = unsafe { MessageMutInner::wrap_raw(raw) };
        inner.into()
    }
    fn __unstable_as_raw_message_mut(&mut self) -> *mut std::ffi::c_void {
        self.get_raw_message_mut(Private).as_ptr() as *mut _
    }
}
