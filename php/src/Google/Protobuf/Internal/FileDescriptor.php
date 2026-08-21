<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

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
            $file->addEnumType(
                EnumDescriptor::buildFromProto(
                    $enum_proto,
                    $proto,
                    ""));
        }
        return $file;
    }
}
