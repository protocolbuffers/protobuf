// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::runtime::InnerExtensionId;
use crate::__internal::{EntityType, SealedInternal};
use crate::codegen_traits::entity_tag::*;
use crate::{
    AsMut, AsView, Enum, IntoMut, IntoView, Message, Mut, MutProxied, ProtoBytes, ProtoString,
    Proxied, ProxiedInRepeated, Repeated, View,
};
use std::marker::PhantomData;

// Does this need to be SealedInternal?  I ran into a problem because ProtoStr is not
// SealedInternal.
pub trait ExtensionKind: Proxied {
    /// The type that holds the default value. This must be `Copy` so it can be
    /// stored in a `const`.
    type DefaultType: Copy;
}

/// A unique identifier for an extension field. Values of this type are emitted into the generated
/// code and used as keys for extension fields.
///
/// All methods that manipulate extension fields (eg. `has_extension`, `get_extension`, etc.)
/// expect to be called with an `ExtensionId` value that identifies the extension field to operate
/// on.
///
/// The type parameter `Extendee` is the message type that the extension field is extending.
///
/// The type parameter `T` defines the kind of extension field: either `T` for a singular extension
/// or `Repeated<T>` for a repeated extension.
pub struct ExtensionId<Extendee, T: ExtensionKind> {
    number: u32,
    pub(crate) inner: InnerExtensionId,
    pub(crate) default: <T as ExtensionKind>::DefaultType,
    phantom: PhantomData<Extendee>,
}

impl<Extendee, T: ExtensionKind> ExtensionId<Extendee, T> {
    pub const fn number(&self) -> u32 {
        self.number
    }
}

pub(crate) trait SingularTypeHelper<T> {}
impl<T: EntityType<Tag = MessageTag>> SingularTypeHelper<MessageTag> for T {}
impl<T: EntityType<Tag = EnumTag>> SingularTypeHelper<EnumTag> for T {}
impl<T: EntityType<Tag = PrimitiveTag>> SingularTypeHelper<PrimitiveTag> for T {}

pub(crate) trait MessageTypeHelper<T> {}
impl<T: EntityType<Tag = MessageTag>> MessageTypeHelper<MessageTag> for T {}

pub(crate) trait EnumTypeHelper<T> {}
impl<T: EntityType<Tag = EnumTag>> EnumTypeHelper<EnumTag> for T {}

pub trait SingularType: EntityType {}
impl<T> SingularType for T where T: EntityType + SingularTypeHelper<T::Tag> {}

pub trait MessageType {}
impl<T> MessageType for T where T: EntityType + MessageTypeHelper<T::Tag> {}

pub trait EnumType {}
impl<T> EnumType for T where T: EntityType + EnumTypeHelper<T::Tag> {}

impl<T: SingularType + ProxiedInRepeated> ExtensionKind for Repeated<T> {
    type DefaultType = ();
}

#[doc(hidden)]
pub trait DefaultTypeHelper<Tag> {
    #[doc(hidden)]
    type DefaultTypeInternal: Copy;
}

// For messages, we do not need to store anything, because the default can be derived from the type.
impl<T> DefaultTypeHelper<MessageTag> for T
where
    T: EntityType<Tag = MessageTag>,
{
    type DefaultTypeInternal = ();
}

// For enums, we need to store the enum value.
impl<T> DefaultTypeHelper<EnumTag> for T
where
    T: EntityType<Tag = EnumTag> + Copy,
{
    type DefaultTypeInternal = T;
}

// Single blanket implementation for ExtensionKind for types T
impl<T: Proxied> ExtensionKind for T
where
    T: SealedInternal + EntityType + DefaultTypeHelper<T::Tag>,
{
    type DefaultType = <T as DefaultTypeHelper<T::Tag>>::DefaultTypeInternal;
}

// For primitives, we need to store the primitive value.
macro_rules! impl_primitive_default {
    ($($t:ty),*) => {
        $(
            impl ExtensionKind for $t {
                type DefaultType = $t;
            }
        )*
    }
}

impl_primitive_default!(i32, u32, i64, u64, f32, f64, bool);

// For strings and bytes, we need to store a static byte slice.

impl ExtensionKind for ProtoString {
    type DefaultType = &'static [u8];
}

impl ExtensionKind for ProtoBytes {
    type DefaultType = &'static [u8];
}

// The "public" API (public to generated code) for creating extension IDs.

pub const fn new_extension_id<Extendee, T: ExtensionKind>(
    number: u32,
    default: T::DefaultType,
    inner: InnerExtensionId,
) -> ExtensionId<Extendee, T> {
    ExtensionId { number, inner, default, phantom: PhantomData }
}

#[doc(hidden)]
pub trait ExtHas<Msg: Message> {
    fn has(&self, msg: impl AsView<Proxied = Msg>) -> bool;
}
#[doc(hidden)]
pub trait ExtGet<Extendee: Message, T: Proxied, Tag> {
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, T>;
}
#[doc(hidden)]
pub trait ExtClear<Msg: Message> {
    fn clear(&self, msg: impl AsMut<MutProxied = Msg>);
}
#[doc(hidden)]
pub trait ExtSet<Msg: Message, Value, Tag> {
    fn set(&self, msg: impl AsMut<MutProxied = Msg>, value: Value);
}
#[doc(hidden)]
pub trait ExtGetMut<Extendee: Message, T: MutProxied, Tag> {
    fn get_mut<'msg>(&self, msg: impl IntoMut<'msg, MutProxied = Extendee>) -> Mut<'msg, T>;
}

impl<Extendee: Message, T: ExtensionKind + crate::codegen_traits::EntityType>
    ExtensionId<Extendee, T>
{
    pub fn has(&self, msg: impl AsView<Proxied = Extendee>) -> bool
    where
        Self: ExtHas<Extendee>,
    {
        ExtHas::has(self, msg)
    }

    pub fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, T>
    where
        Self: ExtGet<Extendee, T, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtGet::get(self, msg)
    }

    pub fn clear(&self, mut msg: impl AsMut<MutProxied = Extendee>)
    where
        Self: ExtClear<Extendee>,
    {
        ExtClear::clear(self, msg)
    }

    pub fn set<Value>(&self, mut msg: impl AsMut<MutProxied = Extendee>, value: Value)
    where
        Self: ExtSet<Extendee, Value, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtSet::set(self, msg, value)
    }

    pub fn get_mut<'msg>(&self, msg: impl IntoMut<'msg, MutProxied = Extendee>) -> Mut<'msg, T>
    where
        T: MutProxied,
        Self: ExtGetMut<Extendee, T, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtGetMut::get_mut(self, msg)
    }
}
