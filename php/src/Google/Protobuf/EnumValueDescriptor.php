<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

class EnumValueDescriptor
{
    private $name;
    private $number;
    private $internal_desc;

    /**
     * @internal
     */
    public function __construct($name, $number, $internal_desc = null)
    {
        $this->name = $name;
        $this->number = $number;
        $this->internal_desc = $internal_desc;
    }

    /**
     * @return string
     */
    public function getName()
    {
        return $this->name;
    }

    /**
     * @return int
     */
    public function getNumber()
    {
        return $this->number;
    }
}
