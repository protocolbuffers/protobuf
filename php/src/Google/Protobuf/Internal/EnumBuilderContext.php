<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\EnumDescriptor;
use Google\Protobuf\EnumValueDescriptor;

class EnumBuilderContext
{

    private $descriptor;
    private $pool;

    public function __construct($full_name, $klass, $pool)
    {
        $this->descriptor = new EnumDescriptor();
        $this->descriptor->setFullName($full_name);
        $this->descriptor->setClass($klass);
        $this->pool = $pool;
    }

    public function value($name, $number)
    {
        $value = new EnumValueDescriptor($name, $number);
        $this->descriptor->addValue($number, $value);
        return $this;
    }

    public function finalizeToPool()
    {
        $this->pool->addEnumDescriptor($this->descriptor);
    }
}
