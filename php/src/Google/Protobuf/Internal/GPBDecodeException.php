<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class GPBDecodeException extends \Exception
{
    public function __construct(
        $message,
        $code = 0,
        \Exception $previous = null)
    {
        parent::__construct(
            "Error occurred during parsing: " . $message,
            $code,
            $previous);
    }
}
