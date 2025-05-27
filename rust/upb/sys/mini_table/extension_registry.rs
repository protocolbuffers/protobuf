// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

mod sys {
    pub use super::super::super::*;
}

use core::ptr::NonNull;
use sys::opaque_pointee::opaque_pointee;

opaque_pointee!(upb_ExtensionRegistry);
pub type RawExtensionRegistry = NonNull<upb_ExtensionRegistry>;
