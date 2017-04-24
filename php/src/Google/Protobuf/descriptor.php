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

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\MessageOptions;

class FileDescriptor
{

    private $package;
    private $message_type = [];
    private $enum_type = [];

    public function setPackage($package)
    {
        $this->package = $package;
    }

    public function getPackage()
    {
        return $this->package;
    }

    public function getMessageType()
    {
        return $this->message_type;
    }

    public function addMessageType($desc)
    {
        $this->message_type[] = $desc;
    }

    public function getEnumType()
    {
        return $this->enum_type;
    }

    public function addEnumType($desc)
    {
        $this->enum_type[]= $desc;
    }

    public static function buildFromProto($proto)
    {
        $file = new FileDescriptor();
        $file->setPackage($proto->getPackage());
        foreach ($proto->getMessageType() as $message_proto) {
            $file->addMessageType(Descriptor::buildFromProto(
                $message_proto, $proto, ""));
        }
        foreach ($proto->getEnumType() as $enum_proto) {
            $file->getEnumType()[] =
                $file->addEnumType(
                    EnumDescriptor::buildFromProto(
                        $enum_proto,
                        $proto,
                        ""));
        }
        return $file;
    }
}

class Descriptor
{

    private $full_name;
    private $field = [];
    private $nested_type = [];
    private $enum_type = [];
    private $klass;
    private $options;
    private $oneof_decl = [];

    public function addOneofDecl($oneof)
    {
        $this->oneof_decl[] = $oneof;
    }

    public function getOneofDecl()
    {
        return $this->oneof_decl;
    }

    public function setFullName($full_name)
    {
        $this->full_name = $full_name;
    }

    public function getFullName()
    {
        return $this->full_name;
    }

    public function addField($field)
    {
        $this->field[$field->getNumber()] = $field;
    }

    public function getField()
    {
        return $this->field;
    }

    public function addNestedType($desc)
    {
        $this->nested_type[] = $desc;
    }

    public function getNestedType()
    {
        return $this->nested_type;
    }

    public function addEnumType($desc)
    {
        $this->enum_type[] = $desc;
    }

    public function getEnumType()
    {
        return $this->enum_type;
    }

    public function getFieldByNumber($number)
    {
      if (!isset($this->field[$number])) {
        return NULL;
      } else {
        return $this->field[$number];
      }
    }

    public function setClass($klass)
    {
        $this->klass = $klass;
    }

    public function getClass()
    {
        return $this->klass;
    }

    public function setOptions($options)
    {
        $this->options = $options;
    }

    public function getOptions()
    {
        return $this->options;
    }

    public static function buildFromProto($proto, $file_proto, $containing)
    {
        $desc = new Descriptor();

        $message_name_without_package  = "";
        $classname = "";
        $fullname = "";
        getFullClassName(
            $proto,
            $containing,
            $file_proto,
            $message_name_without_package,
            $classname,
            $fullname);
        $desc->setFullName($fullname);
        $desc->setClass($classname);
        $desc->setOptions($proto->getOptions());

        foreach ($proto->getField() as $field_proto) {
            $desc->addField(FieldDescriptor::buildFromProto($field_proto));
        }

        // Handle nested types.
        foreach ($proto->getNestedType() as $nested_proto) {
            $desc->addNestedType(Descriptor::buildFromProto(
              $nested_proto, $file_proto, $message_name_without_package));
        }

        // Handle nested enum.
        foreach ($proto->getEnumType() as $enum_proto) {
            $desc->addEnumType(EnumDescriptor::buildFromProto(
              $enum_proto, $file_proto, $message_name_without_package));
        }

        // Handle oneof fields.
        foreach ($proto->getOneofDecl() as $oneof_proto) {
            $desc->addOneofDecl(
                OneofDescriptor::buildFromProto($oneof_proto, $desc));
        }

        return $desc;
    }
}

function getClassNamePrefix(
    $classname,
    $file_proto)
{
    $option = $file_proto->getOptions();
    $prefix = is_null($option) ? "" : $option->getPhpClassPrefix();
    if ($prefix !== "") {
        return $prefix;
    }

    $reserved_words = array("Empty");
    foreach ($reserved_words as $reserved_word) {
        if ($classname === $reserved_word) {
            if ($file_proto->getPackage() === "google.protobuf") {
                return "GPB";
            } else {
                return "PB";
            }
        }
    }

    return "";
}

function getClassNameWithoutPackage(
    $name,
    $file_proto)
{
    $classname = implode('_', array_map('ucwords', explode('.', $name)));
    return getClassNamePrefix($classname, $file_proto) . $classname;
}

function getFullClassName(
    $proto,
    $containing,
    $file_proto,
    &$message_name_without_package,
    &$classname,
    &$fullname)
{
    // Full name needs to start with '.'.
    $message_name_without_package = $proto->getName();
    if ($containing !== "") {
        $message_name_without_package =
            $containing . "." . $message_name_without_package;
    }

    $package = $file_proto->getPackage();
    if ($package === "") {
        $fullname = "." . $message_name_without_package;
    } else {
        $fullname = "." . $package . "." . $message_name_without_package;
    }

    $class_name_without_package =
        getClassNameWithoutPackage($message_name_without_package, $file_proto);
    if ($package === "") {
        $classname = $class_name_without_package;
    } else {
        $classname =
            implode('\\', array_map('ucwords', explode('.', $package))).
            "\\".$class_name_without_package;
    }
}

class OneofDescriptor
{

    private $name;
    private $fields;

    public function setName($name)
    {
        $this->name = $name;
    }

    public function getName()
    {
        return $this->name;
    }

    public function addField(&$field)
    {
        $this->fields[] = $field;
    }

    public function getFields()
    {
        return $this->fields;
    }

    public static function buildFromProto($oneof_proto)
    {
        $oneof = new OneofDescriptor();
        $oneof->setName($oneof_proto->getName());
        return $oneof;
    }
}


class EnumDescriptor
{

    private $klass;
    private $full_name;
    private $value;

    public function setFullName($full_name)
    {
        $this->full_name = $full_name;
    }

    public function getFullName()
    {
        return $this->full_name;
    }

    public function addValue($number, $value)
    {
        $this->value[$number] = $value;
    }

    public function setClass($klass)
    {
        $this->klass = $klass;
    }

    public function getClass()
    {
        return $this->klass;
    }

    public static function buildFromProto($proto, $file_proto, $containing)
    {
        $desc = new EnumDescriptor();

        $enum_name_without_package  = "";
        $classname = "";
        $fullname = "";
        getFullClassName(
            $proto,
            $containing,
            $file_proto,
            $enum_name_without_package,
            $classname,
            $fullname);
        $desc->setFullName($fullname);
        $desc->setClass($classname);

        return $desc;
    }
}

class EnumValueDescriptor
{
}

class FieldDescriptor
{

    private $name;
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

    private static function isTypePackable($field_type)
    {
        return ($field_type !== GPBType::STRING  &&
            $field_type !== GPBType::GROUP   &&
            $field_type !== GPBType::MESSAGE &&
            $field_type !== GPBType::BYTES);
    }

    public static function getFieldDescriptor(
        $name,
        $label,
        $type,
        $number,
        $oneof_index,
        $packed,
        $type_name = null)
    {
        $field = new FieldDescriptor();
        $field->setName($name);
        $camel_name = implode('', array_map('ucwords', explode('_', $name)));
        $field->setGetter('get' . $camel_name);
        $field->setSetter('set' . $camel_name);
        $field->setType($type);
        $field->setNumber($number);
        $field->setLabel($label);
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
        $type_name = null;
        switch ($proto->getType()) {
            case GPBType::MESSAGE:
            case GPBType::GROUP:
            case GPBType::ENUM:
                $type_name = $proto->getTypeName();
                break;
            default:
                break;
        }

        $oneof_index = $proto->hasOneofIndex() ? $proto->getOneofIndex() : -1;
        $packed = false;
        $options = $proto->getOptions();
        if ($options !== null) {
            $packed = $options->getPacked();
        }

        return FieldDescriptor::getFieldDescriptor(
            $proto->getName(), $proto->getLabel(), $proto->getType(),
            $proto->getNumber(), $oneof_index, $packed, $type_name);
    }
}
