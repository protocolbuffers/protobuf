<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
