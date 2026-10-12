<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class EnumValueDescriptor
{
    use HasPublicDescriptorTrait;

    private $name;
    private $number;
    private $custom_json_name;

    public function __construct($name, $number, $custom_json_name = null)
    {
        $this->name = $name;
        $this->number = $number;
        $this->custom_json_name = $custom_json_name;
        $this->public_desc = new \Google\Protobuf\EnumValueDescriptor($this);
    }

    public function getName()
    {
        return $this->name;
    }

    public function getNumber()
    {
        return $this->number;
    }

    /**
     * Returns the explicit custom JSON name if specified via
     * (pb.enumvalue.json), or null if no custom JSON name is set.
     *
     * @return string|null
     */
    public function getCustomJsonName()
    {
        return $this->custom_json_name;
    }
}
