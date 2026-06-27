// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits related to interop with the underlying upb types.
//!
//! These traits are deliberately not available on the prelude, as they should
//! be used rarely and with great care.

use super::*;

/// Provides functionality for conversion to and from a raw `upb_Message*`.
///
/// This trait is empty for the `upb` kernel because interop for owned messages
/// is not yet supported. It requires more work to handle arena allocation.
pub trait OwnedMessageInterop: SealedInternal {}
impl<T: Message> OwnedMessageInterop for T {}

pub trait MessageViewInterop<'msg>: SealedInternal {
    fn __unstable_as_raw_message(&self) -> *const std::ffi::c_void;
    unsafe fn __unstable_wrap_raw_message(raw: &'msg *const std::ffi::c_void) -> Self;
    unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(raw: *const std::ffi::c_void) -> Self;
}

/// Provides functionality for conversion to and from a raw `upb_Message*`.
///
/// This trait is empty for the `upb` kernel because interop for mutable
/// messages is not yet supported. It requires more work to handle arena
/// allocation.
pub trait MessageMutInterop<'msg>: SealedInternal {}
impl<'a, T: MessageMut<'a>> MessageMutInterop<'a> for T {}

impl<'a, T> MessageViewInterop<'a> for T
where
    Self: MessageView<'a> + From<MessageViewInner<'a, <Self as MessageView<'a>>::Message>>,
{
    unsafe fn __unstable_wrap_raw_message(msg: &'a *const std::ffi::c_void) -> Self {
        let raw = RawMessage::new(*msg as *mut _).unwrap();
        let inner = unsafe { MessageViewInner::wrap_raw(raw) };
        inner.into()
    }
    unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(msg: *const std::ffi::c_void) -> Self {
        let raw = RawMessage::new(msg as *mut _).unwrap();
        let inner = unsafe { MessageViewInner::wrap_raw(raw) };
        inner.into()
    }
    fn __unstable_as_raw_message(&self) -> *const std::ffi::c_void {
        self.get_ptr(Private).raw().as_ptr() as *const _
    }
}
