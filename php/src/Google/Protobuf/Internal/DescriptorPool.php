<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
    private $unique_descs = [];
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
        $this->unique_descs[$descriptor->getFullName()] =
            $descriptor;
        $this->class_to_desc[$descriptor->getClass()] = $descriptor;
        $this->class_to_desc[$descriptor->getLegacyClass()] = $descriptor;
        $this->class_to_desc[$descriptor->getPreviouslyUnreservedClass()] = $descriptor;
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
        foreach ($this->unique_descs as $klass => $desc) {
            $this->crossLink($desc);
        }
        unset($desc);
    }
}
