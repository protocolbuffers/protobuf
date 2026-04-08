// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits that are implemented by codegen types.

use crate::__internal::runtime::{
    KernelMessage, KernelMessageMut, KernelMessageView, MessageMutInterop, MessageViewInterop,
    OwnedMessageInterop,
};
use crate::__internal::SealedInternal;
use crate::AsMut;
use crate::AsView;
use crate::IntoMut;
use crate::IntoView;
use crate::{MutProxied, ProtoBytes, ProtoString};
use create::Parse;
use read::Serialize;
use std::fmt::Debug;
use write::{Clear, ClearAndParse, CopyFrom, MergeFrom, TakeFrom};

/// A trait that all generated owned message types implement.
pub trait Message: SealedInternal
  + MutProxied
  + for<'a> MutProxied<View<'a> = Self::MessageView<'a>, Mut<'a> = Self::MessageMut<'a>>
  // Create traits:
  + Parse + Default
  // Read traits:
  + Debug + Serialize
  // Write traits:
  + Clear + ClearAndParse + TakeFrom + CopyFrom + MergeFrom
  // Thread safety:
  + Send + Sync
  // Copy/Clone:
  + Clone
  // C++ interop:
  + OwnedMessageInterop
  // Kernel-specific traits
  + KernelMessage
{
    /// The same type as `<Self as Proxied>::View`. This is defined as a second redundant associated
    /// type and should not be necessary, but the having this available is a hacky workaround
    /// that can appease the trait solver in some cases.
    type MessageView<'msg>: MessageView<'msg, Message = Self>;

    /// The same type as `<Self as Proxied>::Mut`. This is defined as a second redundant associated
    /// type and should not be necessary, but the having this available is a hacky workaround
    /// that can appease the trait solver in some cases.
    type MessageMut<'msg>: MessageMut<'msg, Message = Self>;
}

/// A trait that all generated message views implement.
pub trait MessageView<'msg>: SealedInternal
    + AsView<Proxied = Self::Message>
    + IntoView<'msg, Proxied = Self::Message>
    // Read traits:
    + Debug + Serialize + Default
    // Thread safety:
    + Send + Sync
    // Copy/Clone:
    + Copy + Clone
    // C++ interop:
    + MessageViewInterop<'msg>
    // Kernel-specific traits (including user-visible interop traits):
    + KernelMessageView<'msg>
{
    /// The owned message type that this is a view of.
    type Message: Message;
}

/// A trait that all generated message muts implement.
pub trait MessageMut<'msg>: SealedInternal
    + AsView<Proxied = Self::Message>
    + IntoView<'msg, Proxied = Self::Message>
    + AsMut<MutProxied = Self::Message>
    + IntoMut<'msg, MutProxied = Self::Message>
    // Read traits:
    + Debug + Serialize
    // Write traits:
    + Clear + ClearAndParse + TakeFrom + CopyFrom + MergeFrom
    // Thread safety:
    + Send + Sync
    // Copy/Clone:
    // (Neither)
    // C++ interop:
    + MessageMutInterop<'msg>
    // Kernel-specific traits (including user-visible interop traits):
    + KernelMessageMut<'msg>
{
    /// The owned message type that this is a mut of.
    type Message: Message;
}

/// This trait allows us to associate a tag with each type of protobuf entity. The tag indicates
/// whether the entity is a message, enum, primitive, view proxy, or mut proxy. The main purpose of
/// this is to allow us to have separate blanket implementations of various traits for messages
/// and enums.
pub trait EntityType {
    type Tag;
}

pub mod entity_tag {
    pub struct MessageTag;
    pub struct EnumTag;
    pub struct PrimitiveTag;
    pub struct ViewProxyTag;
    pub struct MutProxyTag;
}

macro_rules! impl_entity_type_for_primitives {
    ($($t:ty,)*) => {
        $(
            impl EntityType for $t {
                type Tag = entity_tag::PrimitiveTag;
            }
        )*
    };
}

impl_entity_type_for_primitives!(f32, f64, i32, u32, i64, u64, bool, ProtoBytes, ProtoString,);

/// Operations related to constructing a message. Only owned messages implement
/// these traits.
pub(crate) mod create {
    use super::SealedInternal;
    pub trait Parse: SealedInternal + Sized {
        fn parse(serialized: &[u8]) -> Result<Self, crate::ParseError>;
        fn parse_dont_enforce_required(serialized: &[u8]) -> Result<Self, crate::ParseError>;
    }

    impl<T> Parse for T
    where
        Self: Default + crate::ClearAndParse,
    {
        fn parse(serialized: &[u8]) -> Result<Self, crate::ParseError> {
            let mut msg = Self::default();
            crate::ClearAndParse::clear_and_parse(&mut msg, serialized).map(|_| msg)
        }

        fn parse_dont_enforce_required(serialized: &[u8]) -> Result<Self, crate::ParseError> {
            let mut msg = Self::default();
            crate::ClearAndParse::clear_and_parse_dont_enforce_required(&mut msg, serialized)
                .map(|_| msg)
        }
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
    use crate::{AsMut, AsView};

    pub trait Clear: SealedInternal {
        fn clear(&mut self);
    }

    pub trait ClearAndParse: SealedInternal {
        fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), crate::ParseError>;
        fn clear_and_parse_dont_enforce_required(
            &mut self,
            data: &[u8],
        ) -> Result<(), crate::ParseError>;
    }

    /// Copies the contents from `src` into `self`.
    ///
    /// This is a copy in the sense that `src` message is not mutated and `self` will have
    /// the same state as `src` after this call; it may not be a bitwise copy.
    pub trait CopyFrom: AsView + SealedInternal {
        fn copy_from(&mut self, src: impl AsView<Proxied = Self::Proxied>);
    }

    /// Moves the contents from `src` into `self`.
    ///
    /// Any previous state of `self` is discarded, and if `src` is still observable then it is
    /// guaranteed to be in its default state after this call. If `src` is a field on a parent
    /// message, the presence of that field will be unaffected.
    pub trait TakeFrom: AsView + SealedInternal {
        fn take_from(&mut self, src: impl AsMut<MutProxied = Self::Proxied>);
    }

    pub trait MergeFrom: AsView + SealedInternal {
        fn merge_from(&mut self, src: impl AsView<Proxied = Self::Proxied>);
    }
}
