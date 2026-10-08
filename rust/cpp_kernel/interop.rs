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
//!
//! The raw pointers vended and accepted by these traits are only guaranteed to
//! point to a `proto2::MessageLite`. The pointer is only guaranteed to be a full `proto2::Message*`
//! if the Rust type also implements the `WithReflection` trait. Casting a pointer to
//! `proto2::Message*` for a type that does not implement `WithReflection` is
//! undefined behavior.

use super::*;

/// Methods for converting to and from a raw, owned C++ message pointer.
pub trait OwnedMessageInterop: SealedInternal {
    /// Drops `self` and returns an underlying pointer that it was wrapping
    /// without deleting it. The pointer is a `MessageLite*` in C++ (and only a
    /// `Message*` if `Self` implements `WithReflection`, see the module docs).
    ///
    /// The caller is responsible for ensuring the returned pointer is
    /// subsequently deleted (eg by moving it into a std::unique_ptr in
    /// C++), or else it will leak.
    fn __unstable_leak_raw_message(self) -> *mut std::ffi::c_void;

    /// Takes exclusive ownership of the `raw_message`.
    ///
    /// # Safety
    ///   - The underlying message must be for the same type as `Self`
    ///   - The pointer passed in must not be used by the caller after being passed here (must not
    ///     be read, written, or deleted)
    unsafe fn __unstable_take_ownership_of_raw_message(raw_message: *mut std::ffi::c_void) -> Self;
}

/// Methods for converting to and from a raw C++ message view pointer.
pub trait MessageViewInterop<'msg>: SealedInternal {
    /// Borrows `self` as an underlying C++ raw pointer.
    ///
    /// Note that the returned Value must be used under the same constraints
    /// as though it were a borrow of `self`: it should be treated as a
    /// `const MessageLite*` in C++ (and only as a `const Message*` if `Self`
    /// implements `WithReflection`, see the module docs), and not be mutated
    /// in any way, and any mutation to the parent message may invalidate it,
    /// and it must not be deleted.
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
    ///   - The underlying message must be alive for 'msg and not mutated while the wrapper is live.
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
    ///   - The underlying message must be alive for the caller-chosen 'msg and not mutated while
    ///     the wrapper is live.
    unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(raw: *const std::ffi::c_void) -> Self;

    /// Converts this view into a reference to the underlying C++ `proto2::MessageLite`.
    ///
    /// This is most commonly used to pass messages to C++ functions which accept
    /// `const proto2::MessageLite&` or `const proto2::MessageLite*`.
    ///
    /// The returned `T` is the Rust binding for `proto2::MessageLite` (see
    /// [`CppMessageLiteType`]). It is normally inferred from the parameter type
    /// of the C++ function being called.
    ///
    /// To get a mutable reference, see [`MessageMutInterop::as_cpp_message_lite_mut`].
    #[allow(clippy::wrong_self_convention)]
    fn as_cpp_message_lite<T: CppMessageLiteType>(self) -> &'msg T
    where
        Self: Sized,
    {
        // SAFETY: `self` is a view, so the message is live and not mutated, other than through
        // C++ `mutable` members, for `'msg`. With the C++ kernel, the raw message pointer is a
        // `proto2::MessageLite*`, so we can reinterpret it as a `&'msg proto2::MessageLite`.
        unsafe { &*(self.__unstable_as_raw_message() as *const T) }
    }
}

// TODO: Add an equivalent `CppMessageType` and
// `as_cpp_message{,_mut}` (bounded on `WithReflection`) once `proto2::Message`
// has Crubit bindings.
/// A trait to be implemented exclusively by the Rust binding for the C++ `proto2::MessageLite`
/// class (and not subclasses).
///
/// This trait is used to allow [`MessageViewInterop::as_cpp_message_lite`] and
/// [`MessageMutInterop::as_cpp_message_lite_mut`] to cast views and muts to the underlying C++
/// `proto2::MessageLite` type without creating a build-time dependency from this crate on the
/// `proto2` bindings.
///
/// There is not yet an equivalent for `proto2::Message` (which does not yet
/// have Crubit bindings, and would require a `WithReflection` bound since
/// messages using the lite runtime are not `proto2::Message`s). C++ APIs that
/// need reflection can take a `proto2::MessageLite` and use
/// `proto2::DynamicCastMessage<proto2::Message>`.
///
/// # Safety
///
/// This trait must only be implemented by the Crubit-generated Rust binding for
/// `proto2::MessageLite` so that `as_cpp_message_lite{,_mut}` can reinterpret the
/// `proto2::MessageLite*` of any message borrowed by a view or mut as a `&Self` or
/// `Pin<&mut Self>`.
///
/// Additionally, the Crubit binding of `proto2::MessageLite` must continue to meet these
/// requirements: it has the same layout as the C++ class, is `!Unpin`, and
/// only contains `Cell<MaybeUninit<u8>>` storage.
pub unsafe trait CppMessageLiteType {}

/// Methods for converting to and from a raw, mutable C++ message pointer.
pub trait MessageMutInterop<'msg>: SealedInternal {
    /// Exclusive borrows `self` as an underlying mutable C++ raw pointer.
    ///
    /// Note that the returned Value must be used under the same constraints
    /// as though it were a mut borrow of `self`: it should be treated as a
    /// non-owned `MessageLite*` in C++ (and only as a `Message*` if `Self`
    /// implements `WithReflection`, see the module docs). And any mutation to
    /// the parent message may invalidate it, and it must not be deleted.
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
    ///   - The underlying message must be alive for 'msg and not read or mutated while the wrapper
    ///     is live.
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
    ///   - The underlying message must be alive for the caller-chosen 'msg and not mutated while
    ///     the wrapper is live.
    unsafe fn __unstable_wrap_raw_message_mut_unchecked_lifetime(
        raw: *mut std::ffi::c_void,
    ) -> Self;

    /// Converts this mut into a mutable reference to the underlying C++ `proto2::MessageLite`.
    ///
    /// This is most commonly used to pass messages to C++ functions which accept
    /// `proto2::MessageLite&` or `proto2::MessageLite*`.
    ///
    /// The returned `T` is the Rust binding for `proto2::MessageLite` (see
    /// [`CppMessageLiteType`]). It is normally inferred from the parameter type
    /// of the C++ function being called.
    ///
    /// To get a shared reference, see [`MessageViewInterop::as_cpp_message_lite`].
    #[allow(clippy::wrong_self_convention)]
    fn as_cpp_message_lite_mut<T: CppMessageLiteType>(mut self) -> std::pin::Pin<&'msg mut T>
    where
        Self: Sized,
    {
        // SAFETY: `self` is mut, so the message is live and exclusively borrowed for `'msg`.
        //
        // With the C++ kernel, the raw message pointer is a
        // `proto2::MessageLite*`, which `T: CppMessageLiteType` allows
        // reinterpreting as a `Pin<&'msg mut T>` under these conditions. The
        // message is a C++ object that Rust never moves, so pinning it is sound.
        unsafe {
            std::pin::Pin::new_unchecked(&mut *(self.__unstable_as_raw_message_mut() as *mut T))
        }
    }
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
