// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This is a test including a single public header to ensure things build.
// It helps test that imports are complete/ordered correctly.

#import "GPBType.pbobjc.h"

// Something in the body of this file so the compiler/linker won't complain
// about an empty .o file.
__attribute__((visibility("default"))) char dummy_symbol_22 = 0;
