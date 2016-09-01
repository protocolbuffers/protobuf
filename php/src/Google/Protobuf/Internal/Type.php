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

class GPBInteger
{
    public $high = 0;
    public $low = 0;

    public function __construct($value = 0)
    {
        $this->low = $value & 0xFFFFFFFF;
        if (PHP_INT_SIZE === 8) {
            $this->high = ($value >> 32) & 0xFFFFFFFF;
        }
    }

    // Return 0 for unsigned integers and 1 for signed integers.
    protected function sign()
    {
        trigger_error("Not implemented", E_ERROR);
    }

    public function leftShift($count)
    {
        if ($count > 63) {
            $this->low = 0;
            $this->high = 0;
            return;
        }
        if ($count > 32) {
            $this->high = $this->low;
            $this->low = 0;
            $count -= 32;
        }
        $mask = (1 << $count) - 1;
        $this->high = (($this->high << $count) & 0xFFFFFFFF) |
                  (($this->low >> (32 - $count)) & $mask);
        $this->low = ($this->low << $count) & 0xFFFFFFFF;

        $this->high &= 0xFFFFFFFF;
        $this->low &= 0xFFFFFFFF;
        return $this;
    }

    public function rightShift($count)
    {
        $sign = (($this->high & 0x80000000) >> 31) & $this->sign();
        if ($count > 63) {
            $this->low = -$sign;
            $this->high = -$sign;
            return;
        }
        if ($count > 32) {
            $this->low = $this->high;
            $this->high = -$sign;
            $count -= 32;
        }
        $this->low = (($this->low >> $count) & 0xFFFFFFFF) |
                 (($this->high << (32 - $count)) & 0xFFFFFFFF);
        $this->high = (($this->high >> $count) | (-$sign << $count));

        $this->high &= 0xFFFFFFFF;
        $this->low &= 0xFFFFFFFF;

        return $this;
    }

    public function bitOr($var)
    {
        $this->high |= $var->high;
        $this->low |= $var->low;
        return $this;
    }

    public function bitXor($var)
    {
        $this->high ^= $var->high;
        $this->low ^= $var->low;
        return $this;
    }

    public function bitAnd($var)
    {
        $this->high &= $var->high;
        $this->low &= $var->low;
        return $this;
    }

    // Even: all zero; Odd: all one.
    public function oddMask()
    {
        $low = (-($this->low & 1)) & 0xFFFFFFFF;
        $high = $low;
        return UInt64::newValue($high, $low);
    }

    public function toInteger()
    {
        if (PHP_INT_SIZE === 8) {
            return ($this->high << 32) | $this->low;
        } else {
            return $this->low;
        }
    }

    public function copy()
    {
        return static::newValue($this->high, $this->low);
    }
}

class Uint64 extends GPBInteger
{

    public static function newValue($high, $low)
    {
        $uint64 = new Uint64(0);
        $uint64->high = $high;
        $uint64->low = $low;
        return $uint64;
    }

    protected function sign()
    {
        return 0;
    }
}

class Int64 extends GPBInteger
{

    public static function newValue($high, $low)
    {
        $int64 = new Int64(0);
        $int64->high = $high;
        $int64->low = $low;
        return $int64;
    }

    protected function sign()
    {
        return 1;
    }
}
