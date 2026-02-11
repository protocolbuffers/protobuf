// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Prelude for the Protobuf Rust API.
//!
//! This module contains only the proto! macro and traits which define very
//! common fns that on messages (fns that would be methods on a base class in
//! other languages).
//!
//! All traits here have `Proto` prefixed on them, as the intent of this prelude
//! is to make the methods callable on message instances: if the traits are
//! named for generic reasons, they should be explicitly imported from the
//! `protobuf::` crate instead.

pub use crate::{
    proto, AsMut as _, AsView as _, Clear as _, ClearAndParse as _, CopyFrom as _, IntoMut as _,
    IntoView as _, MergeFrom as _, Parse as _, Serialize as _, TakeFrom as _,
};
