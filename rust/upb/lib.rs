// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(not(bzl), allow(unused_imports))]

mod arena;
pub use arena::Arena;

mod associated_mini_table;
pub use associated_mini_table::AssociatedMiniTable;

mod owned_arena_box;
pub use owned_arena_box::OwnedArenaBox;

#[cfg(bzl)]
pub extern crate upb_sys as sys;
#[cfg(not(bzl))]
pub use sys;

pub mod text;

pub mod wire;
