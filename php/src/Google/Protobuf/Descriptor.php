<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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
