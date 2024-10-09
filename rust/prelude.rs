// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Prelude for the Protobuf Rust API.
//! This module contains only non-struct items that are needed for extremely
//! common usages of the generated types from application code. Any struct
//! or less common items should be imported normally.

pub use crate::{
    proto, AsMut, AsView, Clear, ClearAndParse, IntoMut, IntoView, MergeFrom, Message, MessageMut,
    MessageView, Parse, Serialize,
};
