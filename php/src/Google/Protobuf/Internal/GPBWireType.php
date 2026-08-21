<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class GPBWireType
{
    const VARINT           = 0;
    const FIXED64          = 1;
    const LENGTH_DELIMITED = 2;
    const START_GROUP      = 3;
    const END_GROUP        = 4;
    const FIXED32          = 5;
}
