<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class CodedOutputStream
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

    public function writeVarint32($value, $trim)
    {
        $bytes = str_repeat(chr(0), self::MAX_VARINT64_BYTES);
        $size = self::writeVarintToArray($value, $bytes, $trim);
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
        return $this->writeVarint32($tag, true);
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

    public static function writeVarintToArray($value, &$buffer, $trim = false)
    {
        $current = 0;

        $high = 0;
        $low = 0;
        if (PHP_INT_SIZE == 4) {
            GPBUtil::divideInt64ToInt32($value, $high, $low, $trim);
        } else {
            $low = $value;
        }

        while (($low >= 0x80 || $low < 0) || $high != 0) {
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
