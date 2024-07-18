// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits that are implemeted by codegen types.

use crate::Proxied;
use std::fmt::Debug;

pub trait Message: Default + Debug + Proxied + Send + Sync {}

pub trait MessageView: Debug + Send + Sync {}

pub trait MessageMut: Debug + Sync {}
