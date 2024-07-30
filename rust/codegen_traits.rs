// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits that are implemeted by codegen types.

use crate::__internal::SealedInternal;
use crate::__runtime::RawMessage;
use crate::{MutProxied, MutProxy, ViewProxy};
use create::Parse;
use interop::{MessageMutInterop, MessageViewInterop, OwnedMessageInterop};
use read::Serialize;
use std::fmt::Debug;
use write::{Clear, ClearAndParse};

/// A trait that all generated owned message types implement.
pub trait Message: SealedInternal
  + MutProxied
  // Create traits:
  + Parse + Default
  // Read traits:
  + Debug + Serialize
  // Write traits:
  + Clear + ClearAndParse
  // Thread safety:
  + Send + Sync
  // Copy/Clone:
  + Clone
  // C++ Interop:
  + OwnedMessageInterop
{
}

/// A trait that all generated message views implement.
pub trait MessageView<'msg>: SealedInternal
    + ViewProxy<'msg, Proxied = Self::Message>
    // Read traits:
    + Debug + Serialize
    // Thread safety:
    + Send + Sync
    // Copy/Clone:
    + Copy + Clone
    // C++ Interop:
    + MessageViewInterop<'msg>
{
    #[doc(hidden)]
    type Message: Message;
}

/// A trait that all generated message muts implement.
pub trait MessageMut<'msg>: SealedInternal
    + MutProxy<'msg, MutProxied = Self::Message>
    // Read traits:
    + Debug + Serialize
    // Write traits:
    // TODO: MsgMut should impl ClearAndParse.
    + Clear
    // Thread safety:
    + Sync
    // Copy/Clone:
    // (Neither)
    // C++ Interop:
    + MessageMutInterop<'msg>
{
    #[doc(hidden)]
    type Message: Message;
}

/// Operations related to constructing a message. Only owned messages implement
/// these traits.
pub(crate) mod create {
    use super::SealedInternal;
    pub trait Parse: SealedInternal + Sized {
        fn parse(serialized: &[u8]) -> Result<Self, crate::ParseError>;
    }
}

/// Operations related to reading some aspect of a message (methods that would
/// have a `&self` receiver on an owned message). Owned messages, views, and
/// muts all implement these traits.
pub(crate) mod read {
    use super::SealedInternal;

    pub trait Serialize: SealedInternal {
        fn serialize(&self) -> Result<Vec<u8>, crate::SerializeError>;
    }
}

/// Operations related to mutating a message (methods that would have a `&mut
/// self` receiver on an owned message). Owned messages and muts implement these
/// traits.
pub(crate) mod write {
    use super::SealedInternal;

    pub trait Clear: SealedInternal {
        fn clear(&mut self);
    }

    pub trait ClearAndParse: SealedInternal {
        fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), crate::ParseError>;
    }
}

/// Traits related to interop with C or C++.
///
/// These traits are deliberately not available on the prelude, as they should
/// be used rarely and with great care.
pub(crate) mod interop {
    use super::{RawMessage, SealedInternal};

    /// Traits related to owned message interop. Note that these trait fns
    /// are only available on C++ kernel as upb interop of owned messages
    /// requires more work to handle the Arena behavior.
    pub trait OwnedMessageInterop: SealedInternal {
        /// Drops `self` and returns the `RawMessage` that it was wrapping
        /// without deleting it.
        ///
        /// The caller is responsible for ensuring the returned RawMessage is
        /// subsequently deleted (eg by moving it into a std::unique_ptr in
        /// C++), or else it will leak.
        #[cfg(cpp_kernel)]
        fn __unstable_leak_raw_message(self) -> RawMessage;

        /// Takes exclusive ownership of the `raw_message`.
        ///
        /// # Safety
        ///   - The underlying message must be for the same type as `Self`
        ///   - The pointer passed in must not be used by the caller after being
        ///     passed here (must not be read, written, or deleted)
        #[cfg(cpp_kernel)]
        unsafe fn __unstable_take_ownership_of_raw_message(raw_message: RawMessage) -> Self;
    }

    /// Traits related to message view interop.
    pub trait MessageViewInterop<'msg>: SealedInternal {
        /// Borrows `self` as an underlying `RawMessage`.
        ///
        /// Note that the returned Value must be used under the same constraints
        /// as though it were a borrow of `self`: it should be treated as a
        /// `const Message*` in C++, and not be mutated in any way, and any
        /// mutation to the parent message may invalidate it, and it
        /// must not be deleted.
        fn __unstable_as_raw_message(&self) -> RawMessage;

        /// Wraps the provided `RawMessage` as a MessageView.
        ///
        /// # Safety
        ///   - The underlying message must be for the same type as `Self`
        ///   - The underlying message must be alive for 'msg and not mutated
        ///     while the wrapper is live.
        unsafe fn __unstable_wrap_raw_message(raw: &'msg RawMessage) -> Self;
    }

    /// Traits related to message mut interop. Note that these trait fns
    /// are only available on C++ kernel as upb interop of owned messages
    /// requires more work to handle the Arena behavior.
    pub trait MessageMutInterop<'msg>: SealedInternal {
        /// Exclusive borrows `self` as a `RawMessage`.
        ///
        /// Note that the returned Value must be used under the same constraints
        /// as though it were a mut borrow of `self`: it should be treated as a
        /// non-owned `Message*` in C++. And any mutation to the parent message
        /// may invalidate it, and it must not be deleted.
        #[cfg(cpp_kernel)]
        fn __unstable_as_raw_message_mut(&mut self) -> RawMessage;

        /// Wraps the provided `RawMessage` as a MessageMut.
        ///
        /// # Safety
        ///   - The underlying message must be for the same type as `Self`
        ///   - The underlying message must be alive for 'msg and not read or
        ///     mutated while the wrapper is live.
        #[cfg(cpp_kernel)]
        unsafe fn __unstable_wrap_raw_message_mut(raw: &'msg mut RawMessage) -> Self;
    }
}
