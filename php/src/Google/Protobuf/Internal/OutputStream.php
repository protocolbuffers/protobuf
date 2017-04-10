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

class OutputStream
{

    private $buffer;
    private $buffer_size;
    private $current;

    const MAX_VARINT64_BYTES = 10;

    public function __construct($size)
    {
        $this->current = 0;
        $this->buffer_size = $size;
        $this->buffer = str_repeat(chr(0), $this->buffer_size);
    }

    public function getData()
    {
        return $this->buffer;
    }

    public function writeVarint32($value)
    {
        $bytes = str_repeat(chr(0), self::MAX_VARINT64_BYTES);
        $size = self::writeVarintToArray($value, $bytes);
        return $this->writeRaw($bytes, $size);
    }

    public function writeVarint64($value)
    {
        $bytes = str_repeat(chr(0), self::MAX_VARINT64_BYTES);
        $size = self::writeVarintToArray($value, $bytes);
        return $this->writeRaw($bytes, $size);
    }

    public function writeLittleEndian32($value)
    {
        $bytes = str_repeat(chr(0), 4);
        $size = self::writeLittleEndian32ToArray($value, $bytes);
        return $this->writeRaw($bytes, $size);
    }

    public function writeLittleEndian64($value)
    {
        $bytes = str_repeat(chr(0), 8);
        $size = self::writeLittleEndian64ToArray($value, $bytes);
        return $this->writeRaw($bytes, $size);
    }

    public function writeTag($tag)
    {
        return $this->writeVarint32($tag);
    }

    public function writeRaw($data, $size)
    {
        if ($this->buffer_size < $size) {
            trigger_error("Output stream doesn't have enough buffer.");
            return false;
        }

        for ($i = 0; $i < $size; $i++) {
            $this->buffer[$this->current] = $data[$i];
            $this->current++;
            $this->buffer_size--;
        }
        return true;
    }

    private static function writeVarintToArray($value, &$buffer)
    {
        $current = 0;

        $high = 0;
        $low = 0;
        if (PHP_INT_SIZE == 4) {
            GPBUtil::divideInt64ToInt32($value, $high, $low);
        } else {
            $low = $value;
        }

        while ($low >= 0x80 || $low < 0) {
            $buffer[$current] = chr($low | 0x80);
            $value = ($value >> 7) & ~(0x7F << ((PHP_INT_SIZE << 3) - 7));
            $carry = ($high & 0x7F) << ((PHP_INT_SIZE << 3) - 7);
            $high = ($high >> 7) & ~(0x7F << ((PHP_INT_SIZE << 3) - 7));
            $low = (($low >> 7) & ~(0x7F << ((PHP_INT_SIZE << 3) - 7)) | $carry);
            $current++;
        }
        $buffer[$current] = chr($low);
        return $current + 1;
    }

    private static function writeLittleEndian32ToArray($value, &$buffer)
    {
        $buffer[0] = chr($value & 0x000000FF);
        $buffer[1] = chr(($value >> 8) & 0x000000FF);
        $buffer[2] = chr(($value >> 16) & 0x000000FF);
        $buffer[3] = chr(($value >> 24) & 0x000000FF);
        return 4;
    }

    private static function writeLittleEndian64ToArray($value, &$buffer)
    {
        $high = 0;
        $low = 0;
        if (PHP_INT_SIZE == 4) {
            GPBUtil::divideInt64ToInt32($value, $high, $low);
        } else {
            $low = $value & 0xFFFFFFFF;
            $high = ($value >> 32) & 0xFFFFFFFF;
        }

        $buffer[0] = chr($low & 0x000000FF);
        $buffer[1] = chr(($low >> 8) & 0x000000FF);
        $buffer[2] = chr(($low >> 16) & 0x000000FF);
        $buffer[3] = chr(($low >> 24) & 0x000000FF);
        $buffer[4] = chr($high & 0x000000FF);
        $buffer[5] = chr(($high >> 8) & 0x000000FF);
        $buffer[6] = chr(($high >> 16) & 0x000000FF);
        $buffer[7] = chr(($high >> 24) & 0x000000FF);
        return 8;
    }

}
