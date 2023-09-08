// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Runtime-internal macros

/// Defines a `impl SettableValue<$proxied> for SomeType` body that forwards to
/// another implementation.
///
/// # Example
/// ```ignore
/// impl<'a, const N: usize> SettableValue<[u8]> for &'a [u8; N] {
///     // Use the `SettableValue<[u8]>` implementation for `&[u8]`:
///     impl_forwarding_settable_value!([u8], self => &self[..]);
/// }
/// ```
macro_rules! impl_forwarding_settable_value {
    ($proxied:ty, $self:ident => $self_forwarding_expr:expr) => {
        fn set_on(
            $self,
            _private: $crate::__internal::Private,
            mutator: $crate::Mut<'_, $proxied>,
        ) {
            ($self_forwarding_expr).set_on(Private, mutator)
        }

        fn set_on_absent(
            $self,
            _private: $crate::__internal::Private,
            absent_mutator: <$proxied as $crate::ProxiedWithPresence>::AbsentMutData<'_>,
        ) -> <$proxied as $crate::ProxiedWithPresence>::PresentMutData<'_> {
            ($self_forwarding_expr).set_on_absent($crate::__internal::Private, absent_mutator)
        }

        fn set_on_present(
            $self,
            _private: $crate::__internal::Private,
            present_mutator: <$proxied as $crate::ProxiedWithPresence>::PresentMutData<'_>,
        ) {
            ($self_forwarding_expr).set_on_present($crate::__internal::Private, present_mutator)
        }
    };
}
pub(crate) use impl_forwarding_settable_value;
