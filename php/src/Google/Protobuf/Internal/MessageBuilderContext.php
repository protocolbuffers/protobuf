<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\GPBLabel;
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\Descriptor;
use Google\Protobuf\Internal\FieldDescriptor;

class MessageBuilderContext
{

    private $descriptor;
    private $pool;

    public function __construct($full_name, $klass, $pool)
    {
        $this->descriptor = new Descriptor();
        $this->descriptor->setFullName($full_name);
        $this->descriptor->setClass($klass);
        $this->pool = $pool;
    }

    private function getFieldDescriptor($name, $label, $type,
                                      $number, $type_name = null)
    {
        $field = new FieldDescriptor();
        $field->setName($name);
        $camel_name = implode('', array_map('ucwords', explode('_', $name)));
        $field->setGetter('get' . $camel_name);
        $field->setSetter('set' . $camel_name);
        $field->setType($type);
        $field->setNumber($number);
        $field->setLabel($label);

        // At this time, the message/enum type may have not been added to pool.
        // So we use the type name as place holder and will replace it with the
        // actual descriptor in cross building.
        switch ($type) {
        case GPBType::MESSAGE:
          $field->setMessageType($type_name);
          break;
        case GPBType::ENUM:
          $field->setEnumType($type_name);
          break;
        default:
          break;
        }

        return $field;
    }

    public function optional($name, $type, $number, $type_name = null)
    {
        $this->descriptor->addField($this->getFieldDescriptor(
            $name,
            GPBLabel::OPTIONAL,
            $type,
            $number,
            $type_name));
        return $this;
    }

    public function repeated($name, $type, $number, $type_name = null)
    {
        $this->descriptor->addField($this->getFieldDescriptor(
            $name,
            GPBLabel::REPEATED,
            $type,
            $number,
            $type_name));
        return $this;
    }

    public function required($name, $type, $number, $type_name = null)
    {
        $this->descriptor->addField($this->getFieldDescriptor(
            $name,
            GPBLabel::REQUIRED,
            $type,
            $number,
            $type_name));
        return $this;
    }

    public function finalizeToPool()
    {
        $this->pool->addDescriptor($this->descriptor);
    }
}
