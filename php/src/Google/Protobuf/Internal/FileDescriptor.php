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
