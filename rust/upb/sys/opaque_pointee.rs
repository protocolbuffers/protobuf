// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Macro to create structs that will act as opaque pointees. These structs are
// never intended to be dereferenced in Rust.
// This is a workaround until stabilization of [`extern type`].
// TODO: convert to extern type once stabilized.
// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
macro_rules! opaque_pointee {
    ($name: ident) => {
        #[repr(C)]
        pub struct $name {
            _data: [u8; 0],
            _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
        }
    };
}

pub(crate) use opaque_pointee;
