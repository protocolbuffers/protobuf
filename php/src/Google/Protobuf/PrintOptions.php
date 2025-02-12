<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

class PrintOptions
{
    const PRESERVE_PROTO_FIELD_NAMES = 1 << 0;
    const ALWAYS_PRINT_ENUMS_AS_INTS = 1 << 1;
}