<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class GPBType
{
    const DOUBLE   =  1;
    const FLOAT    =  2;
    const INT64    =  3;
    const UINT64   =  4;
    const INT32    =  5;
    const FIXED64  =  6;
    const FIXED32  =  7;
    const BOOL     =  8;
    const STRING   =  9;
    const GROUP    = 10;
    const MESSAGE  = 11;
    const BYTES    = 12;
    const UINT32   = 13;
    const ENUM     = 14;
    const SFIXED32 = 15;
    const SFIXED64 = 16;
    const SINT32   = 17;
    const SINT64   = 18;
}
