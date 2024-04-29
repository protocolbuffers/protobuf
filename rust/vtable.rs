// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::{Private, PtrAndLen, RawMessage};
use crate::__runtime::{copy_bytes_in_arena_if_needed_by_runtime, MutatorMessageRef};
use crate::{
    AbsentField, FieldEntry, Mut, MutProxy, Optional, PresentField, Proxied, ProxiedWithPresence,
    View, ViewProxy,
};
use std::fmt::{self, Debug};

/// A proxied type that can use a vtable to provide get/set access for a
/// present field.
///
/// This vtable should consist of `unsafe fn`s that call thunks that operate on
/// `RawMessage`. The structure of this vtable is different per proxied type.
pub trait ProxiedWithRawVTable: Proxied {
    /// The vtable for get/set access, stored in static memory.
    type VTable: Debug + 'static;

    fn make_view(_private: Private, mut_inner: RawVTableMutator<'_, Self>) -> View<'_, Self>;
    fn make_mut(_private: Private, inner: RawVTableMutator<'_, Self>) -> Mut<'_, Self>;
}

/// A proxied type that can use a vtable to provide get/set/clear access for
/// an optional field.
///
/// This vtable should consist of `unsafe fn`s that call thunks that operate on
/// `RawMessage`. The structure of this vtable is different per-proxied type.
pub trait ProxiedWithRawOptionalVTable: ProxiedWithRawVTable + ProxiedWithPresence {
    /// The vtable for get/set/clear, must contain `Self::VTable`.
    type OptionalVTable: Debug + 'static;

    /// Cast from a static reference of `OptionalVTable` to `VTable`.
    /// This should mean `OptionalVTable` contains a `VTable`.
    fn upcast_vtable(
        _private: Private,
        optional_vtable: &'static Self::OptionalVTable,
    ) -> &'static Self::VTable;
}

/// Constructs a new field entry from a raw message, a vtable for manipulation,
/// and an eager check for whether the value is present or not.
///
/// # Safety
/// - `msg_ref` must be valid to provide as an argument for `vtable`'s methods
///   for `'msg`.
/// - If given `msg_ref` as an argument, any values returned by `vtable` methods
///   must be valid for `'msg`.
/// - Operations on the vtable must be thread-compatible.
#[doc(hidden)]
pub unsafe fn new_vtable_field_entry<'msg, T: ProxiedWithRawOptionalVTable + ?Sized>(
    _private: Private,
    msg_ref: MutatorMessageRef<'msg>,
    optional_vtable: &'static T::OptionalVTable,
    is_set: bool,
) -> FieldEntry<'msg, T>
where
    T: ProxiedWithPresence<
            PresentMutData<'msg> = RawVTableOptionalMutatorData<'msg, T>,
            AbsentMutData<'msg> = RawVTableOptionalMutatorData<'msg, T>,
        >,
{
    let data = RawVTableOptionalMutatorData { msg_ref, vtable: optional_vtable };
    if is_set {
        Optional::Set(PresentField::from_inner(Private, data))
    } else {
        Optional::Unset(AbsentField::from_inner(Private, data))
    }
}

/// The internal implementation type for a vtable-based `protobuf::Mut<T>`.
///
/// This stores the two components necessary to mutate the field:
/// borrowed message data and a vtable reference.
///
/// The borrowed message data varies per runtime: C++ needs a message pointer,
/// while UPB needs a message pointer and an `&Arena`.
///
/// Implementations of `ProxiedWithRawVTable` implement get/set
/// on top of `RawVTableMutator<T>`, and the top-level mutator (e.g.
/// `BytesMut`) calls these methods.
///
/// [`RawVTableOptionalMutatorData`] is similar, but also includes the
/// capability to has/clear.
pub struct RawVTableMutator<'msg, T: ProxiedWithRawVTable + ?Sized> {
    msg_ref: MutatorMessageRef<'msg>,
    vtable: &'static T::VTable,
}

// These use manual impls instead of derives to avoid unnecessary bounds on `T`.
// This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/
impl<'msg, T: ProxiedWithRawVTable + ?Sized> Clone for RawVTableMutator<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: ProxiedWithRawVTable + ?Sized> Copy for RawVTableMutator<'msg, T> {}

impl<'msg, T: ProxiedWithRawVTable + ?Sized> Debug for RawVTableMutator<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RawVTableMutator")
            .field("msg_ref", &self.msg_ref)
            .field("vtable", &self.vtable)
            .finish()
    }
}

impl<'msg, T: ProxiedWithRawVTable + ?Sized> RawVTableMutator<'msg, T> {
    /// # Safety
    /// - `msg_ref` must be valid to provide as an argument for `vtable`'s
    ///   methods for `'msg`.
    /// - If given `msg_ref` as an argument, any values returned by `vtable`
    ///   methods must be valid for `'msg`.
    #[doc(hidden)]
    pub unsafe fn new(
        _private: Private,
        msg_ref: MutatorMessageRef<'msg>,
        vtable: &'static T::VTable,
    ) -> Self {
        RawVTableMutator { msg_ref, vtable }
    }
}

/// [`RawVTableMutator`], but also includes has/clear.
///
/// This is used as the `PresentData` and `AbsentData` for `impl
/// ProxiedWithPresence for T`. In that implementation, `clear_present_field`
/// and `set_absent_to_default` will use methods implemented on
/// `RawVTableOptionalMutatorData<T>` to do the setting and clearing.
///
/// This has the same representation for "present" and "absent" data;
/// differences like default values are obviated by the vtable.
pub struct RawVTableOptionalMutatorData<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> {
    msg_ref: MutatorMessageRef<'msg>,
    vtable: &'static T::OptionalVTable,
}

unsafe impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> Sync
    for RawVTableOptionalMutatorData<'msg, T>
{
}

// These use manual impls instead of derives to avoid unnecessary bounds on `T`.
// This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/
impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> Clone
    for RawVTableOptionalMutatorData<'msg, T>
{
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> Copy
    for RawVTableOptionalMutatorData<'msg, T>
{
}

impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> Debug
    for RawVTableOptionalMutatorData<'msg, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RawVTableOptionalMutatorData")
            .field("msg_ref", &self.msg_ref)
            .field("vtable", &self.vtable)
            .finish()
    }
}

impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized> RawVTableOptionalMutatorData<'msg, T> {
    /// # Safety
    /// - `msg_ref` must be valid to provide as an argument for `vtable`'s
    ///   methods for `'msg`.
    /// - If given `msg_ref` as an argument, any values returned by `vtable`
    ///   methods must be valid for `'msg`.
    #[doc(hidden)]
    pub unsafe fn new(
        _private: Private,
        msg_ref: MutatorMessageRef<'msg>,
        vtable: &'static T::OptionalVTable,
    ) -> Self {
        Self { msg_ref, vtable }
    }

    fn into_raw_mut(self) -> RawVTableMutator<'msg, T> {
        RawVTableMutator { msg_ref: self.msg_ref, vtable: T::upcast_vtable(Private, self.vtable) }
    }
}

impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized + 'msg> ViewProxy<'msg>
    for RawVTableOptionalMutatorData<'msg, T>
{
    type Proxied = T;

    fn as_view(&self) -> View<'_, T> {
        T::make_view(Private, self.into_raw_mut())
    }

    fn into_view<'shorter>(self) -> View<'shorter, T>
    where
        'msg: 'shorter,
    {
        T::make_view(Private, self.into_raw_mut())
    }
}

// Note: though this raw value implements `MutProxy`, the `as_mut` is only valid
// when the field is known to be present. `FieldEntry` enforces this in its
// design: `AbsentField { inner: RawVTableOptionalMutatorData<T> }` does not
// implement `MutProxy`.
impl<'msg, T: ProxiedWithRawOptionalVTable + ?Sized + 'msg> MutProxy<'msg>
    for RawVTableOptionalMutatorData<'msg, T>
{
    fn as_mut(&mut self) -> Mut<'_, T> {
        T::make_mut(Private, self.into_raw_mut())
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, T>
    where
        'msg: 'shorter,
    {
        T::make_mut(Private, self.into_raw_mut())
    }
}

impl ProxiedWithRawVTable for [u8] {
    type VTable = BytesMutVTable;

    fn make_view(_private: Private, mut_inner: RawVTableMutator<'_, Self>) -> View<'_, Self> {
        mut_inner.get()
    }

    fn make_mut(_private: Private, inner: RawVTableMutator<'_, Self>) -> Mut<'_, Self> {
        crate::string::BytesMut::from_inner(Private, inner)
    }
}

impl ProxiedWithRawOptionalVTable for [u8] {
    type OptionalVTable = BytesOptionalMutVTable;
    fn upcast_vtable(
        _private: Private,
        optional_vtable: &'static Self::OptionalVTable,
    ) -> &'static Self::VTable {
        &optional_vtable.base
    }
}

/// A generic thunk vtable for mutating a present primitive field.
#[doc(hidden)]
#[derive(Debug)]
pub struct PrimitiveVTable<T> {
    pub(crate) setter: unsafe extern "C" fn(msg: RawMessage, val: T),
    pub(crate) getter: unsafe extern "C" fn(msg: RawMessage) -> T,
}

impl<T> PrimitiveVTable<T> {
    #[doc(hidden)]
    pub const fn new(
        _private: Private,
        getter: unsafe extern "C" fn(msg: RawMessage) -> T,
        setter: unsafe extern "C" fn(msg: RawMessage, val: T),
    ) -> Self {
        Self { getter, setter }
    }
}

macro_rules! impl_raw_vtable_mutator_get_set {
  ($($t:ty),*) => {
      $(
          impl RawVTableMutator<'_, $t> {
              pub(crate) fn get(self) -> $t {
                  // SAFETY:
                  // - `msg_ref` is valid for the lifetime of `RawVTableMutator` as promised by the
                  //   caller of `new`.
                  unsafe { (self.vtable.getter)(self.msg_ref.msg()) }
              }

              /// # Safety
              /// - `msg_ref` must be valid for the lifetime of `RawVTableMutator`.
              pub(crate) unsafe fn set(self, val: $t) {
                // SAFETY:
                // - `msg_ref` is valid for the lifetime of `RawVTableMutator` as promised by the
                //   caller of `new`.
                  unsafe { (self.vtable.setter)(self.msg_ref.msg(), val) }
              }
          }
      )*
  }
}

impl_raw_vtable_mutator_get_set!(bool, f32, f64, i32, i64, u32, u64);

/// A generic thunk vtable for mutating a present `bytes` or `string` field.
#[doc(hidden)]
#[derive(Debug)]
pub struct BytesMutVTable {
    pub(crate) setter: unsafe extern "C" fn(msg: RawMessage, val: PtrAndLen),
    pub(crate) getter: unsafe extern "C" fn(msg: RawMessage) -> PtrAndLen,
}

/// A generic thunk vtable for mutating an `optional` `bytes` or `string` field.
#[derive(Debug)]
pub struct BytesOptionalMutVTable {
    pub(crate) base: BytesMutVTable,
    pub(crate) clearer: unsafe extern "C" fn(msg: RawMessage),
    pub(crate) default: &'static [u8],
}

impl BytesMutVTable {
    #[doc(hidden)]
    pub const fn new(
        _private: Private,
        getter: unsafe extern "C" fn(msg: RawMessage) -> PtrAndLen,
        setter: unsafe extern "C" fn(msg: RawMessage, val: PtrAndLen),
    ) -> Self {
        Self { getter, setter }
    }
}

impl BytesOptionalMutVTable {
    /// # Safety
    /// The `default` value must be UTF-8 if required by
    /// the runtime and this is for a `string` field.
    #[doc(hidden)]
    pub const unsafe fn new(
        _private: Private,
        getter: unsafe extern "C" fn(msg: RawMessage) -> PtrAndLen,
        setter: unsafe extern "C" fn(msg: RawMessage, val: PtrAndLen),
        clearer: unsafe extern "C" fn(msg: RawMessage),
        default: &'static [u8],
    ) -> Self {
        Self { base: BytesMutVTable { getter, setter }, clearer, default }
    }
}

impl<'msg> RawVTableMutator<'msg, [u8]> {
    pub(crate) fn get(self) -> &'msg [u8] {
        // SAFETY:
        // - `msg_ref` is valid for `'msg` as promised by the caller of `new`.
        // - The caller of `BytesMutVTable` promised that the returned `PtrAndLen` is
        //   valid for `'msg`.
        unsafe { (self.vtable.getter)(self.msg_ref.msg()).as_ref() }
    }

    /// # Safety
    /// - `msg_ref` must be valid for `'msg`
    /// - If this is for a `string` field, `val` must be valid UTF-8 if the
    ///   runtime requires it.
    pub(crate) unsafe fn set(self, val: &[u8]) {
        let val = copy_bytes_in_arena_if_needed_by_runtime(self.msg_ref, val);
        // SAFETY:
        // - `msg_ref` is valid for `'msg` as promised by the caller of `new`.
        unsafe { (self.vtable.setter)(self.msg_ref.msg(), val.into()) }
    }

    pub(crate) fn truncate(&self, len: usize) {
        if len == 0 {
            // SAFETY: The empty string is valid UTF-8.
            unsafe {
                self.set(b"");
            }
            return;
        }
        todo!("b/294252563")
    }
}

impl<'msg> RawVTableOptionalMutatorData<'msg, [u8]> {
    /// Sets an absent `bytes`/`string` field to its default value.
    pub(crate) fn set_absent_to_default(self) -> Self {
        // SAFETY: The default value is UTF-8 if required by the
        // runtime as promised by the caller of `BytesOptionalMutVTable::new`.
        unsafe { self.set(self.vtable.default) }
    }

    /// # Safety
    /// - If this is a `string` field, `val` must be valid UTF-8 if required by
    ///   the runtime.
    pub(crate) unsafe fn set(self, val: &[u8]) -> Self {
        let val = copy_bytes_in_arena_if_needed_by_runtime(self.msg_ref, val);
        // SAFETY:
        // - `msg_ref` is valid for `'msg` as promised by the caller.
        unsafe { (self.vtable.base.setter)(self.msg_ref.msg(), val.into()) }
        self
    }

    pub(crate) fn clear(self) -> Self {
        // SAFETY:
        // - `msg_ref` is valid for `'msg` as promised by the caller.
        // - The caller of `new` promised that the returned `PtrAndLen` is valid for
        //   `'msg`.
        unsafe { (self.vtable.clearer)(self.msg_ref.msg()) }
        self
    }
}
