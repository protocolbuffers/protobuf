// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use core::ptr::NonNull;

opaque_pointee!(upb_ExtensionRegistry);
pub type RawExtensionRegistry = NonNull<upb_ExtensionRegistry>;
