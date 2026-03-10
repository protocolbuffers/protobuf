<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

use Google\Protobuf\Internal\GetPublicDescriptorTrait;

class Descriptor
{
    use GetPublicDescriptorTrait;

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
        return trim($this->internal_desc->getFullName(), ".");
    }

    /**
     * @return string PHP class name
     */
    public function getClass()
    {
        return $this->internal_desc->getClass();
    }

    /**
     * @param int $index Must be >= 0 and < getFieldCount()
     * @return FieldDescriptor
     */
    public function getField($index)
    {
        return $this->getPublicDescriptor($this->internal_desc->getFieldByIndex($index));
    }

    /**
     * @return int Number of fields in message
     */
    public function getFieldCount()
    {
        return count($this->internal_desc->getField());
    }

    /**
     * @param int $index Must be >= 0 and < getOneofDeclCount()
     * @return OneofDescriptor
     */
    public function getOneofDecl($index)
    {
        return $this->getPublicDescriptor($this->internal_desc->getOneofDecl()[$index]);
    }

    /**
     * @return int Number of oneofs in message
     */
    public function getOneofDeclCount()
    {
        return count($this->internal_desc->getOneofDecl());
    }

    /**
     * @return int Number of real oneofs in message
     */
    public function getRealOneofDeclCount()
    {
        return $this->internal_desc->getRealOneofDeclCount();
    }
}
