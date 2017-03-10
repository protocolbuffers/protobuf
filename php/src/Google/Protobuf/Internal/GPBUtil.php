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
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;

class GPBUtil
{
    public function divideInt64ToInt32($value, &$high, &$low, $trim = false)
    {
        $isNeg = (bccomp($value, 0) < 0);
        if ($isNeg) {
            $value = bcsub(0, $value);
        }

        $high = (int) bcdiv(bcadd($value, 1), 4294967296);
        $low = bcmod($value, 4294967296);
        if (bccomp($low, 2147483647) > 0) {
            $low = (int) bcsub($low, 4294967296);
        } else {
            $low = (int) $low;
        }

        if ($isNeg) {
            $high = ~$high;
            $low = ~$low;
            $low++;
            if (!$low) {
                $high++;
            }
        }

        if ($trim) {
            $high = 0;
        }
    }


    public static function checkString(&$var, $check_utf8)
    {
        if (is_array($var) || is_object($var)) {
            trigger_error("Expect string.", E_USER_ERROR);
            return;
        }
        if (!is_string($var)) {
            $var = strval($var);
        }
        if ($check_utf8 && !preg_match('//u', $var)) {
            trigger_error("Expect utf-8 encoding.", E_USER_ERROR);
            return;
        }
    }

    public static function checkEnum(&$var)
    {
      static::checkInt32($var);
    }

    public static function checkInt32(&$var)
    {
        if (is_numeric($var)) {
            $var = intval($var);
        } else {
            trigger_error("Expect integer.", E_USER_ERROR);
        }
    }

    public static function checkUint32(&$var)
    {
        if (is_numeric($var)) {
            if (PHP_INT_SIZE === 8) {
                $var = intval($var);
                $var |= ((-(($var >> 31) & 0x1)) & ~0xFFFFFFFF);
            } else {
                if (bccomp($var, 0x7FFFFFFF) > 0) {
                    $var = bcsub($var, "4294967296");
                }
                $var = (int) $var;
            }
        } else {
            trigger_error("Expect integer.", E_USER_ERROR);
        }
    }

    public static function checkInt64(&$var)
    {
        if (is_numeric($var)) {
            if (PHP_INT_SIZE == 8) {
                $var = intval($var);
            } else {
                $var = bcdiv($var, 1, 0);
            }
        } else {
            trigger_error("Expect integer.", E_USER_ERROR);
        }
    }

    public static function checkUint64(&$var)
    {
        if (is_numeric($var)) {
            if (PHP_INT_SIZE == 8) {
                $var = intval($var);
            } else {
                $var = bcdiv($var, 1, 0);
            }
        } else {
            trigger_error("Expect integer.", E_USER_ERROR);
        }
    }

    public static function checkFloat(&$var)
    {
        if (is_float($var) || is_numeric($var)) {
            $var = floatval($var);
        } else {
            trigger_error("Expect float.", E_USER_ERROR);
        }
    }

    public static function checkDouble(&$var)
    {
        if (is_float($var) || is_numeric($var)) {
            $var = floatval($var);
        } else {
            trigger_error("Expect float.", E_USER_ERROR);
        }
    }

    public static function checkBool(&$var)
    {
        if (is_array($var) || is_object($var)) {
            trigger_error("Expect boolean.", E_USER_ERROR);
            return;
        }
        $var = boolval($var);
    }

    public static function checkMessage(&$var, $klass)
    {
        if (!$var instanceof $klass && !is_null($var)) {
            trigger_error("Expect message.", E_USER_ERROR);
        }
    }

    public static function checkRepeatedField(&$var, $type, $klass = null)
    {
        if (!$var instanceof RepeatedField && !is_array($var)) {
            trigger_error("Expect array.", E_USER_ERROR);
        }
        if (is_array($var)) {
            $tmp = new RepeatedField($type, $klass);
            foreach ($var as $value) {
                $tmp[] = $value;
            }
            return $tmp;
        } else {
            if ($var->getType() != $type) {
                trigger_error(
                    "Expect repeated field of different type.",
                    E_USER_ERROR);
            }
            if ($var->getType() === GPBType::MESSAGE &&
                $var->getClass() !== $klass) {
                trigger_error(
                    "Expect repeated field of different message.",
                    E_USER_ERROR);
            }
            return $var;
        }
    }

    public static function checkMapField(&$var, $key_type, $value_type, $klass = null)
    {
        if (!$var instanceof MapField && !is_array($var)) {
            trigger_error("Expect dict.", E_USER_ERROR);
        }
        if (is_array($var)) {
            $tmp = new MapField($key_type, $value_type, $klass);
            foreach ($var as $key => $value) {
                $tmp[$key] = $value;
            }
            return $tmp;
        } else {
            if ($var->getKeyType() != $key_type) {
                trigger_error(
                    "Expect map field of key type.",
                    E_USER_ERROR);
            }
            if ($var->getValueType() != $value_type) {
                trigger_error(
                    "Expect map field of value type.",
                    E_USER_ERROR);
            }
            if ($var->getValueType() === GPBType::MESSAGE &&
                $var->getValueClass() !== $klass) {
                trigger_error(
                    "Expect map field of different value message.",
                    E_USER_ERROR);
            }
            return $var;
        }
    }

    public static function Int64($value)
    {
        return new Int64($value);
    }

    public static function Uint64($value)
    {
        return new Uint64($value);
    }
}
