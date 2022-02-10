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

use Google\Protobuf\Internal\Descriptor;
use Google\Protobuf\Internal\FileDescriptor;
use Google\Protobuf\Internal\FileDescriptorSet;
use Google\Protobuf\Internal\MessageBuilderContext;
use Google\Protobuf\Internal\EnumBuilderContext;

class DescriptorPool
{
    private static $pool;
    // Map from message names to sub-maps, which are maps from field numbers to
    // field descriptors.
    private $class_to_desc = [];
    private $class_to_enum_desc = [];
    private $proto_to_class = [];

    public static function getGeneratedPool()
    {
        if (!isset(self::$pool)) {
            self::$pool = new DescriptorPool();
        }
        return self::$pool;
    }

    public function internalAddGeneratedFile($data, $use_nested = false)
    {
        $files = new FileDescriptorSet();
        $files->mergeFromString($data);

        foreach($files->getFile() as $file_proto) {
            $file = FileDescriptor::buildFromProto($file_proto);

            foreach ($file->getMessageType() as $desc) {
                $this->addDescriptor($desc);
            }
            unset($desc);

            foreach ($file->getEnumType() as $desc) {
                $this->addEnumDescriptor($desc);
            }
            unset($desc);

            foreach ($file->getMessageType() as $desc) {
                $this->crossLink($desc);
            }
            unset($desc);
        }
    }

    public function addMessage($name, $klass)
    {
        return new MessageBuilderContext($name, $klass, $this);
    }

    public function addEnum($name, $klass)
    {
        return new EnumBuilderContext($name, $klass, $this);
    }

    public function addDescriptor($descriptor)
    {
        $this->proto_to_class[$descriptor->getFullName()] =
            $descriptor->getClass();
        $this->class_to_desc[$descriptor->getClass()] = $descriptor;
        $this->class_to_desc[$descriptor->getLegacyClass()] = $descriptor;
        foreach ($descriptor->getNestedType() as $nested_type) {
            $this->addDescriptor($nested_type);
        }
        foreach ($descriptor->getEnumType() as $enum_type) {
            $this->addEnumDescriptor($enum_type);
        }
    }

    public function addEnumDescriptor($descriptor)
    {
        $this->proto_to_class[$descriptor->getFullName()] =
            $descriptor->getClass();
        $this->class_to_enum_desc[$descriptor->getClass()] = $descriptor;
        $this->class_to_enum_desc[$descriptor->getLegacyClass()] = $descriptor;
    }

    public function getDescriptorByClassName($klass)
    {
        if (isset($this->class_to_desc[$klass])) {
            return $this->class_to_desc[$klass];
        } else {
            return null;
        }
    }

    public function getEnumDescriptorByClassName($klass)
    {
        if (isset($this->class_to_enum_desc[$klass])) {
            return $this->class_to_enum_desc[$klass];
        } else {
            return null;
        }
    }

    public function getDescriptorByProtoName($proto)
    {
        if (isset($this->proto_to_class[$proto])) {
            $klass = $this->proto_to_class[$proto];
            return $this->class_to_desc[$klass];
        } else {
          return null;
        }
    }

    public function getEnumDescriptorByProtoName($proto)
    {
        $klass = $this->proto_to_class[$proto];
        return $this->class_to_enum_desc[$klass];
    }

    private function crossLink(Descriptor $desc)
    {
        foreach ($desc->getField() as $field) {
            switch ($field->getType()) {
                case GPBType::MESSAGE:
                    $proto = $field->getMessageType();
                    if ($proto[0] == '.') {
                      $proto = substr($proto, 1);
                    }
                    $subdesc = $this->getDescriptorByProtoName($proto);
                    if (is_null($subdesc)) {
                        trigger_error(
                            'proto not added: ' . $proto
                            . " for " . $desc->getFullName(), E_USER_ERROR);
                    }
                    $field->setMessageType($subdesc);
                    break;
                case GPBType::ENUM:
                    $proto = $field->getEnumType();
                    if ($proto[0] == '.') {
                      $proto = substr($proto, 1);
                    }
                    $field->setEnumType(
                        $this->getEnumDescriptorByProtoName($proto));
                    break;
                default:
                    break;
            }
        }
        unset($field);

        foreach ($desc->getNestedType() as $nested_type) {
            $this->crossLink($nested_type);
        }
        unset($nested_type);
    }

    public function finish()
    {
        foreach ($this->class_to_desc as $klass => $desc) {
            $this->crossLink($desc);
        }
        unset($desc);
    }
}
