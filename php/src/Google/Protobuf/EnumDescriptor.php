<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

class EnumDescriptor
{
    private $internal_desc;

    /**
     * @internal
     */
    public function __construct($internal_desc)
    {
        $this->internal_desc = $internal_desc;
    }

    /**
     * @return string Full protobuf message name
     */
    public function getFullName()
    {
        return $this->internal_desc->getFullName();
    }

    /**
     * @return string PHP class name
     */
    public function getClass()
    {
        return $this->internal_desc->getClass();
    }

    /**
     * @param int $index Must be >= 0 and < getValueCount()
     * @return EnumValueDescriptor
     */
    public function getValue($index)
    {
        return $this->internal_desc->getValueDescriptorByIndex($index);
    }

    /**
     * @return int Number of values in enum
     */
    public function getValueCount()
    {
        return $this->internal_desc->getValueCount();
    }
}
