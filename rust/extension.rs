// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::runtime::InnerExtensionId;
use crate::__internal::{EntityType, Private, SealedInternal};
use crate::codegen_traits::entity_tag::*;
use crate::{
    AsMut, AsView, Enum, IntoMut, IntoProxied, IntoView, Message, Mut, MutProxied, ProtoBytes,
    ProtoString, Proxied, ProxiedInRepeated, Repeated, View,
};
use std::marker::PhantomData;

/// A unique identifier for an extension field. Values of this type are emitted into the generated
/// code and used to access extension fields.
///
/// The type parameter `Extendee` is the message type that the extension field is extending.
///
/// The type parameter `T` defines the kind of extension field: either `T` for a singular extension
/// or `Repeated<T>` for a repeated extension.
pub struct ExtensionId<Extendee, T: Proxied> {
    number: u32,
    pub(crate) inner: InnerExtensionId,
    pub(crate) default: Option<View<'static, T>>,
    phantom: PhantomData<Extendee>,
}

impl<Extendee, T: Proxied> ExtensionId<Extendee, T> {
    pub const fn number(&self) -> u32 {
        self.number
    }
}

// The "public" API (public to generated code) for creating extension IDs.

pub const fn new_extension_id<Extendee, T: Proxied>(
    _private: Private,
    number: u32,
    default: View<'static, T>,
    inner: InnerExtensionId,
) -> ExtensionId<Extendee, T> {
    ExtensionId { number, inner, default: Some(default), phantom: PhantomData }
}

// Repeated and message extension IDs do not have defaults.

pub const fn new_repeated_extension_id<Extendee, T: ProxiedInRepeated>(
    _private: Private,
    number: u32,
    inner: InnerExtensionId,
) -> ExtensionId<Extendee, Repeated<T>> {
    ExtensionId { number, inner, default: None, phantom: PhantomData }
}

pub const fn new_message_extension_id<Extendee, T: Message>(
    _private: Private,
    number: u32,
    inner: InnerExtensionId,
) -> ExtensionId<Extendee, T> {
    ExtensionId { number, inner, default: None, phantom: PhantomData }
}

#[doc(hidden)]
pub trait ExtHas<Msg: Message> {
    fn has(&self, _private: Private, msg: impl AsView<Proxied = Msg>) -> bool;
}
#[doc(hidden)]
pub trait ExtClear<Msg: Message> {
    fn clear(&self, _private: Private, msg: impl AsMut<MutProxied = Msg>);
}
#[doc(hidden)]
pub trait ExtAccess<Extendee: Message, T: Proxied, Tag> {
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, T>;
    fn set(
        &self,
        _private: Private,
        msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<T>,
    );
}
#[doc(hidden)]
pub trait ExtGetMut<Extendee: Message, T: MutProxied, Tag> {
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Extendee>,
    ) -> Mut<'msg, T>;
}

impl<Extendee: Message, T: Proxied + crate::codegen_traits::EntityType> ExtensionId<Extendee, T> {
    pub fn has(&self, msg: impl AsView<Proxied = Extendee>) -> bool
    where
        Self: ExtHas<Extendee>,
    {
        ExtHas::has(self, Private, msg)
    }

    pub fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, T>
    where
        Self: ExtAccess<Extendee, T, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtAccess::get(self, Private, msg)
    }

    pub fn clear(&self, mut msg: impl AsMut<MutProxied = Extendee>)
    where
        Self: ExtClear<Extendee>,
    {
        ExtClear::clear(self, Private, msg)
    }

    pub fn set(&self, mut msg: impl AsMut<MutProxied = Extendee>, value: impl IntoProxied<T>)
    where
        Self: ExtAccess<Extendee, T, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtAccess::set(self, Private, msg, value)
    }

    pub fn get_mut<'msg>(&self, msg: impl IntoMut<'msg, MutProxied = Extendee>) -> Mut<'msg, T>
    where
        T: MutProxied,
        Self: ExtGetMut<Extendee, T, <T as crate::codegen_traits::EntityType>::Tag>,
    {
        ExtGetMut::get_mut(self, Private, msg)
    }
}
