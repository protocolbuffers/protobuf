// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{
    IntoProxied, Message, Mut, Proxied, Repeated, View,
    __internal::{Private, SealedInternal},
};

/// Singular types are types which are allowed as a simple field, or in a repeated, or as a map
/// value.
///
/// In typical Protobuf terminology, 'singular' refers to a property of individual field (namely
/// that it is not a repeated or map field), but in this case this Singular trait is implemented
/// for any type which is usable in that position, which is also the same types usable as a repeated
/// or map value.
///
/// Note that a subset of Singular types are permitted as Map keys: messages, bytes and enums are
/// not allowed in that position.
///
/// # Safety
/// - It must be sound to call `*_unchecked*(x)` with an `index` less than
///   `repeated_len(x)`.
pub unsafe trait Singular: Proxied + SealedInternal {
    /// Constructs a new owned `Repeated` field.
    #[doc(hidden)]
    fn repeated_new(_private: Private) -> Repeated<Self>;

    /// Frees the repeated field in-place, for use in `Drop`.
    ///
    /// # Safety
    /// - After `repeated_free`, no other methods on the input are safe to call.
    #[doc(hidden)]
    unsafe fn repeated_free(_private: Private, _repeated: &mut Repeated<Self>);

    /// Gets the length of the repeated field.
    #[doc(hidden)]
    fn repeated_len(_private: Private, repeated: View<Repeated<Self>>) -> usize;

    /// Appends a new element to the end of the repeated field.
    #[doc(hidden)]
    fn repeated_push(_private: Private, repeated: Mut<Repeated<Self>>, val: impl IntoProxied<Self>);

    /// Clears the repeated field of elements.
    #[doc(hidden)]
    fn repeated_clear(_private: Private, repeated: Mut<Repeated<Self>>);

    /// # Safety
    /// `index` must be less than `Self::repeated_len(repeated)`
    #[doc(hidden)]
    unsafe fn repeated_get_unchecked(
        _private: Private,
        repeated: View<Repeated<Self>>,
        index: usize,
    ) -> View<Self>;

    /// # Safety
    /// `index` must be less than `Self::repeated_len(repeated)`
    #[allow(unused_variables)]
    #[doc(hidden)]
    unsafe fn repeated_get_mut_unchecked(
        _private: Private,
        repeated: Mut<Repeated<Self>>,
        index: usize,
    ) -> Mut<Self>
    where
        Self: Message,
    {
        panic!("repeated_get_mut_unchecked is only implemented for messages");
    }

    /// # Safety
    /// `index` must be less than `Self::repeated_len(repeated)`
    #[doc(hidden)]
    unsafe fn repeated_set_unchecked(
        _private: Private,
        repeated: Mut<Repeated<Self>>,
        index: usize,
        val: impl IntoProxied<Self>,
    );

    /// Copies the values in the `src` repeated field into `dest`.
    #[doc(hidden)]
    fn repeated_copy_from(_private: Private, src: View<Repeated<Self>>, dest: Mut<Repeated<Self>>);

    /// Ensures that the repeated field has enough space allocated to insert at
    /// least `additional` values without an allocation.
    #[doc(hidden)]
    fn repeated_reserve(_private: Private, repeated: Mut<Repeated<Self>>, additional: usize);
}
