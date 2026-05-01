// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

pub mod conversions;
pub mod extension;
pub mod interop;
pub mod map;
pub mod message;
pub mod minitable;
pub mod repeated;
pub mod string;

pub use conversions::*;
pub use extension::*;
pub use interop::*;
pub use map::*;
pub use message::*;
pub use minitable::*;
pub use repeated::*;
pub use string::*;

use crate::__internal::entity_tag::*;
use crate::__internal::{EntityType, MatcherEq, Private, SealedInternal};
use crate::{
    AsMut, AsView, Clear, ClearAndParse, CopyFrom, IntoProxied, Map, MapIter, MapMut, MapValue,
    MapView, MergeFrom, Message, MessageMut, MessageView, Mut, ParseError, ProtoBytes, ProtoStr,
    ProtoString, Proxied, Repeated, RepeatedMut, RepeatedView, Serialize, SerializeError, Singular,
    TakeFrom, View,
};
use std::fmt::Debug;
use std::marker::PhantomData;
use std::mem::{size_of, ManuallyDrop, MaybeUninit};
use std::ptr::{self, NonNull};
use std::slice;
use std::sync::OnceLock;

#[cfg(bzl)]
extern crate upb;
#[cfg(not(bzl))]
use crate::upb;

pub use upb::Arena;
pub use upb::AssociatedMiniTable;
pub use upb::AssociatedMiniTableEnum;
pub use upb::MessagePtr;
pub type MiniTablePtr = upb::RawMiniTable;
pub type MiniTableEnumPtr = upb::RawMiniTableEnum;
pub type MiniTableExtensionPtr = upb::RawMiniTableExtension;
pub type ExtensionRegistryPtr = upb::RawExtensionRegistry;
use upb::*;

pub fn debug_string<T: UpbGetMessagePtr>(msg: &T) -> String {
    let ptr = msg.get_ptr(Private);
    // SAFETY: `ptr` is legally dereferenceable.
    unsafe { upb::debug_string(ptr) }
}

pub(crate) type RawRepeatedField = upb::RawArray;
pub(crate) type RawMap = upb::RawMap;
pub(crate) type PtrAndLen = upb::StringView;

/// A trait implemented by types which are allowed as keys in maps.
/// This is all types for fields except for repeated, maps, bytes, messages, enums and floating point types.
/// This trait is defined separately in cpp.rs and upb.rs to be able to set better subtrait bounds.
#[doc(hidden)]
pub trait MapKey: Proxied + EntityType + UpbTypeConversions<Self::Tag> + SealedInternal {}

thread_local! {
    // We need to avoid dropping this Arena, because we use it to build mini tables that
    // effectively have 'static lifetimes.
    pub static THREAD_LOCAL_ARENA: ManuallyDrop<Arena> = ManuallyDrop::new(Arena::new());
}

pub mod __unstable {
    // Stores a serialized FileDescriptorProto, along with references to its dependencies.
    pub struct DescriptorInfo {
        // The serialized FileDescriptorProto.
        pub descriptor: &'static [u8],
        // A reference to the DescriptorInfo associated with each .proto file that the current one
        // imports.
        pub deps: &'static [&'static DescriptorInfo],
    }
}
