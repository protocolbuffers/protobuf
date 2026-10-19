// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

pub mod base;
pub mod mem;
pub mod message;
pub mod mini_table;
pub mod opaque_pointee;
pub mod text;
pub mod wire;

#[cfg(not(bzl))]
pub mod reflection;

#[cfg(test)]
mod test_helpers;
