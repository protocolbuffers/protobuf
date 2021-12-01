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

class FieldDescriptor
{
    use HasPublicDescriptorTrait;

    private $name;
    private $json_name;
    private $setter;
    private $getter;
    private $number;
    private $label;
    private $type;
    private $message_type;
    private $enum_type;
    private $packed;
    private $is_map;
    private $oneof_index = -1;

    public function __construct()
    {
        $this->public_desc = new \Google\Protobuf\FieldDescriptor($this);
    }

    public function setOneofIndex($index)
    {
        $this->oneof_index = $index;
    }

    public function getOneofIndex()
    {
        return $this->oneof_index;
    }

    public function setName($name)
    {
        $this->name = $name;
    }

    public function getName()
    {
        return $this->name;
    }

    public function setJsonName($json_name)
    {
        $this->json_name = $json_name;
    }

    public function getJsonName()
    {
        return $this->json_name;
    }

    public function setSetter($setter)
    {
        $this->setter = $setter;
    }

    public function getSetter()
    {
        return $this->setter;
    }

    public function setGetter($getter)
    {
        $this->getter = $getter;
    }

    public function getGetter()
    {
        return $this->getter;
    }

    public function setNumber($number)
    {
        $this->number = $number;
    }

    public function getNumber()
    {
        return $this->number;
    }

    public function setLabel($label)
    {
        $this->label = $label;
    }

    public function getLabel()
    {
        return $this->label;
    }

    public function isRepeated()
    {
        return $this->label === GPBLabel::REPEATED;
    }

    public function setType($type)
    {
        $this->type = $type;
    }

    public function getType()
    {
        return $this->type;
    }

    public function setMessageType($message_type)
    {
        $this->message_type = $message_type;
    }

    public function getMessageType()
    {
        return $this->message_type;
    }

    public function setEnumType($enum_type)
    {
        $this->enum_type = $enum_type;
    }

    public function getEnumType()
    {
        return $this->enum_type;
    }

    public function setPacked($packed)
    {
        $this->packed = $packed;
    }

    public function getPacked()
    {
        return $this->packed;
    }

    public function isPackable()
    {
        return $this->isRepeated() && self::isTypePackable($this->type);
    }

    public function isMap()
    {
        return $this->getType() == GPBType::MESSAGE &&
               !is_null($this->getMessageType()->getOptions()) &&
               $this->getMessageType()->getOptions()->getMapEntry();
    }

    public function isTimestamp()
    {
        return $this->getType() == GPBType::MESSAGE &&
            $this->getMessageType()->getClass() === "Google\Protobuf\Timestamp";
    }

    public function isWrapperType()
    {
        if ($this->getType() == GPBType::MESSAGE) {
            $class = $this->getMessageType()->getClass();
            return in_array($class, [
                "Google\Protobuf\DoubleValue",
                "Google\Protobuf\FloatValue",
                "Google\Protobuf\Int64Value",
                "Google\Protobuf\UInt64Value",
                "Google\Protobuf\Int32Value",
                "Google\Protobuf\UInt32Value",
                "Google\Protobuf\BoolValue",
                "Google\Protobuf\StringValue",
                "Google\Protobuf\BytesValue",
            ]);
        }
        return false;
    }

    private static function isTypePackable($field_type)
    {
        return ($field_type !== GPBType::STRING  &&
            $field_type !== GPBType::GROUP   &&
            $field_type !== GPBType::MESSAGE &&
            $field_type !== GPBType::BYTES);
    }

    public static function getFieldDescriptor($proto)
    {
        $type_name = null;
        $type = $proto->getType();
        switch ($type) {
            case GPBType::MESSAGE:
            case GPBType::GROUP:
            case GPBType::ENUM:
                $type_name = $proto->getTypeName();
                break;
            default:
                break;
        }

        $oneof_index = $proto->hasOneofIndex() ? $proto->getOneofIndex() : -1;
        // TODO: once proto2 is supported, this default should be false
        // for proto2.
        if ($proto->getLabel() === GPBLabel::REPEATED &&
            $proto->getType() !== GPBType::MESSAGE &&
            $proto->getType() !== GPBType::GROUP &&
            $proto->getType() !== GPBType::STRING &&
            $proto->getType() !== GPBType::BYTES) {
          $packed = true;
        } else {
          $packed = false;
        }
        $options = $proto->getOptions();
        if ($options !== null) {
            $packed = $options->getPacked();
        }

        $field = new FieldDescriptor();
        $field->setName($proto->getName());

        $json_name = $proto->hasJsonName() ? $proto->getJsonName() :
            lcfirst(implode('', array_map('ucwords', explode('_', $proto->getName()))));
        if ($proto->hasJsonName()) {
            $json_name = $proto->getJsonName();
        } else {
            $proto_name = $proto->getName();
            $json_name = implode('', array_map('ucwords', explode('_', $proto_name)));
            if ($proto_name[0] !== "_" && !ctype_upper($proto_name[0])) {
                $json_name = lcfirst($json_name);
            }
        }
        $field->setJsonName($json_name);

        $camel_name = implode('', array_map('ucwords', explode('_', $proto->getName())));
        $field->setGetter('get' . $camel_name);
        $field->setSetter('set' . $camel_name);
        $field->setType($proto->getType());
        $field->setNumber($proto->getNumber());
        $field->setLabel($proto->getLabel());
        $field->setPacked($packed);
        $field->setOneofIndex($oneof_index);

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

    public static function buildFromProto($proto)
    {
        return FieldDescriptor::getFieldDescriptor($proto);
    }
}
