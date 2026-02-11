<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

use Google\Protobuf\Internal\GetPublicDescriptorTrait;
use Google\Protobuf\Internal\GPBType;

class FieldDescriptor
{
    use GetPublicDescriptorTrait;

    /** @var  \Google\Protobuf\Internal\FieldDescriptor $internal_desc */
    private $internal_desc;

    /**
     * @internal
     */
    public function __construct($internal_desc)
    {
        $this->internal_desc = $internal_desc;
    }

    /**
     * @return string Field name
     */
    public function getName()
    {
        return $this->internal_desc->getName();
    }

    /**
     * @return int Protobuf field number
     */
    public function getNumber()
    {
        return $this->internal_desc->getNumber();
    }

    /**
     * @return boolean
     */
    public function isRequired()
    {
        return $this->internal_desc->isRequired();
    }

    /**
     * @return boolean
     */
    public function isRepeated()
    {
        return $this->internal_desc->isRepeated();
    }

    /**
     * @return int
     */
    public function getType()
    {
        return $this->internal_desc->getType();
    }

    /**
     * @return OneofDescriptor
     */
    public function getContainingOneof()
    {
        return $this->getPublicDescriptor($this->internal_desc->getContainingOneof());
    }

    /**
     * Gets the field's containing oneof, only if non-synthetic.
     *
     * @return null|OneofDescriptor
     */
    public function getRealContainingOneof()
    {
        return $this->getPublicDescriptor($this->internal_desc->getRealContainingOneof());
    }

    /**
     * Returns true if this field tracks presence, ie. does the field
     * distinguish between "unset" and "present with default value."
     *
     * This includes required, optional, and oneof fields. It excludes maps,
     * repeated fields, and singular proto3 fields without "optional".
     *
     * For fields where hasPresence() == true, the return value of
     * msg.hasField() is semantically meaningful.
     *
     * @return boolean
     */
    public function hasPresence()
    {
        return $this->internal_desc->hasPresence();
    }

    /**
     * @return Descriptor Returns a descriptor for the field type if the field type is a message, otherwise throws \Exception
     * @throws \Exception
     */
    public function getMessageType()
    {
        if ($this->getType() == GPBType::MESSAGE) {
            return $this->getPublicDescriptor($this->internal_desc->getMessageType());
        } else {
            throw new \Exception("Cannot get message type for non-message field '" . $this->getName() . "'");
        }
    }

    /**
     * @return EnumDescriptor Returns an enum descriptor if the field type is an enum, otherwise throws \Exception
     * @throws \Exception
     */
    public function getEnumType()
    {
        if ($this->getType() == GPBType::ENUM) {
            return $this->getPublicDescriptor($this->internal_desc->getEnumType());
        } else {
            throw new \Exception("Cannot get enum type for non-enum field '" . $this->getName() . "'");
        }
    }

    /**
     * @return boolean
     */
    public function isMap()
    {
        return $this->internal_desc->isMap();
    }
}
