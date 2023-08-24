// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! Runtime-internal macros

/// Defines a set of opaque pointers and a unique non-accessible pointees.
///
/// This provides a type safety benefit over using `NonNull<u8>` everywhere.
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
///
/// Because this defines a new private module, it can only be called once per
/// module.
///
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
macro_rules! define_opaque_nonnulls {
    ($($(#[$meta:meta])* $vis:vis type $name:ident = NonNull<$raw_name:ident>;)*) => {
        mod _opaque_pointees {
            $(
                #[doc = concat!("Opaque pointee for [`", stringify!($name), "`][pointer]")]
                ///
                /// This type is not meant to be dereferenced in Rust code.
                /// It is only meant to provide type safety for raw pointers
                /// which are manipulated behind FFI.
                #[doc = concat!("[pointer]: super::", stringify!($name))]
                #[repr(C)]
                pub struct $raw_name {
                    _data: [u8; 0],
                    _marker: ::std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
                }
            )*
        }
        $(
            $(#[$meta])*
            ///
            /// This is an opaque pointer used for FFI:
            /// do not dereference this type in Rust code.
            $vis type $name = ::std::ptr::NonNull<_opaque_pointees::$raw_name>;
        )*
    };
}
pub(crate) use define_opaque_nonnulls;

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
