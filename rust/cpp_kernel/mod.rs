// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

pub mod extension;
pub mod interop;
pub mod map;
pub mod message;
pub mod raw;
pub mod repeated;
pub mod string;

pub use extension::*;
pub use interop::*;
pub use map::*;
pub use message::*;
pub use raw::*;
pub use repeated::*;
pub use string::*;

use crate::__internal::{Enum, MatcherEq, Private, SealedInternal};
use crate::{
    AsMut, AsView, Clear, ClearAndParse, CopyFrom, IntoProxied, Map, MapIter, MapMut, MapValue,
    MapView, MergeFrom, Message, Mut, MutProxied, ParseError, ProtoBytes, ProtoStr, ProtoString,
    Proxied, Repeated, RepeatedMut, RepeatedView, Serialize, SerializeError, Singular, TakeFrom,
    View,
};
use core::fmt::Debug;
use paste::paste;
use std::convert::identity;
use std::ffi::{c_int, c_void};
use std::fmt;
use std::marker::PhantomData;
use std::mem::{ManuallyDrop, MaybeUninit};
use std::ops::Deref;
use std::ptr::{self, NonNull};
use std::slice;
