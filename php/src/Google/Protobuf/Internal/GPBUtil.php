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

use Google\Protobuf\Duration;
use Google\Protobuf\FieldMask;
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;

function camel2underscore($input) {
    preg_match_all(
        '!([A-Z][A-Z0-9]*(?=$|[A-Z][a-z0-9])|[A-Za-z][a-z0-9]+)!',
        $input,
        $matches);
    $ret = $matches[0];
    foreach ($ret as &$match) {
        $match = $match == strtoupper($match) ? strtolower($match) : lcfirst($match);
    }
    return implode('_', $ret);
}

class GPBUtil
{
    const NANOS_PER_MILLISECOND = 1000000;
    const NANOS_PER_MICROSECOND = 1000;
    const TYPE_URL_PREFIX = 'type.googleapis.com/';

    public static function divideInt64ToInt32($value, &$high, &$low, $trim = false)
    {
        $isNeg = (bccomp($value, 0) < 0);
        if ($isNeg) {
            $value = bcsub(0, $value);
        }

        $high = bcdiv($value, 4294967296);
        $low = bcmod($value, 4294967296);
        if (bccomp($high, 2147483647) > 0) {
            $high = (int) bcsub($high, 4294967296);
        } else {
            $high = (int) $high;
        }
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
                $high = (int)($high + 1);
            }
        }

        if ($trim) {
            $high = 0;
        }
    }

    public static function checkString(&$var, $check_utf8)
    {
        if (is_array($var) || is_object($var)) {
            throw new \InvalidArgumentException("Expect string.");
        }
        if (!is_string($var)) {
            $var = strval($var);
        }
        if ($check_utf8 && !preg_match('//u', $var)) {
            throw new \Exception("Expect utf-8 encoding.");
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
            throw new \Exception("Expect integer.");
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
            throw new \Exception("Expect integer.");
        }
    }

    public static function checkInt64(&$var)
    {
        if (is_numeric($var)) {
            if (PHP_INT_SIZE == 8) {
                $var = intval($var);
            } else {
                if (is_float($var) ||
                    is_integer($var) ||
                    (is_string($var) &&
                         bccomp($var, "9223372036854774784") < 0)) {
                    $var = number_format($var, 0, ".", "");
                }
            }
        } else {
            throw new \Exception("Expect integer.");
        }
    }

    public static function checkUint64(&$var)
    {
        if (is_numeric($var)) {
            if (PHP_INT_SIZE == 8) {
                $var = intval($var);
            } else {
                $var = number_format($var, 0, ".", "");
            }
        } else {
            throw new \Exception("Expect integer.");
        }
    }

    public static function checkFloat(&$var)
    {
        if (is_float($var) || is_numeric($var)) {
            $var = unpack("f", pack("f", $var))[1];
        } else {
            throw new \Exception("Expect float.");
        }
    }

    public static function checkDouble(&$var)
    {
        if (is_float($var) || is_numeric($var)) {
            $var = floatval($var);
        } else {
            throw new \Exception("Expect float.");
        }
    }

    public static function checkBool(&$var)
    {
        if (is_array($var) || is_object($var)) {
            throw new \Exception("Expect boolean.");
        }
        $var = boolval($var);
    }

    public static function checkMessage(&$var, $klass, $newClass = null)
    {
        if (!$var instanceof $klass && !is_null($var)) {
            throw new \Exception("Expect $klass.");
        }
    }

    public static function checkRepeatedField(&$var, $type, $klass = null)
    {
        if (!$var instanceof RepeatedField && !is_array($var)) {
            throw new \Exception("Expect array.");
        }
        if (is_array($var)) {
            $tmp = new RepeatedField($type, $klass);
            foreach ($var as $value) {
                $tmp[] = $value;
            }
            return $tmp;
        } else {
            if ($var->getType() != $type) {
                throw new \Exception(
                    "Expect repeated field of different type.");
            }
            if ($var->getType() === GPBType::MESSAGE &&
                $var->getClass() !== $klass &&
                $var->getLegacyClass() !== $klass) {
                throw new \Exception(
                    "Expect repeated field of " . $klass . ".");
            }
            return $var;
        }
    }

    public static function checkMapField(&$var, $key_type, $value_type, $klass = null)
    {
        if (!$var instanceof MapField && !is_array($var)) {
            throw new \Exception("Expect dict.");
        }
        if (is_array($var)) {
            $tmp = new MapField($key_type, $value_type, $klass);
            foreach ($var as $key => $value) {
                $tmp[$key] = $value;
            }
            return $tmp;
        } else {
            if ($var->getKeyType() != $key_type) {
                throw new \Exception("Expect map field of key type.");
            }
            if ($var->getValueType() != $value_type) {
                throw new \Exception("Expect map field of value type.");
            }
            if ($var->getValueType() === GPBType::MESSAGE &&
                $var->getValueClass() !== $klass &&
                $var->getLegacyValueClass() !== $klass) {
                throw new \Exception(
                    "Expect map field of " . $klass . ".");
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

    public static function getClassNamePrefix(
        $classname,
        $file_proto)
    {
        $option = $file_proto->getOptions();
        $prefix = is_null($option) ? "" : $option->getPhpClassPrefix();
        if ($prefix !== "") {
            return $prefix;
        }

        $reserved_words = array(
            "abstract"=>0, "and"=>0, "array"=>0, "as"=>0, "break"=>0,
            "callable"=>0, "case"=>0, "catch"=>0, "class"=>0, "clone"=>0,
            "const"=>0, "continue"=>0, "declare"=>0, "default"=>0, "die"=>0,
            "do"=>0, "echo"=>0, "else"=>0, "elseif"=>0, "empty"=>0,
            "enddeclare"=>0, "endfor"=>0, "endforeach"=>0, "endif"=>0,
            "endswitch"=>0, "endwhile"=>0, "eval"=>0, "exit"=>0, "extends"=>0,
            "final"=>0, "for"=>0, "foreach"=>0, "function"=>0, "global"=>0,
            "goto"=>0, "if"=>0, "implements"=>0, "include"=>0,
            "include_once"=>0, "instanceof"=>0, "insteadof"=>0, "interface"=>0,
            "isset"=>0, "list"=>0, "namespace"=>0, "new"=>0, "or"=>0,
            "print"=>0, "private"=>0, "protected"=>0, "public"=>0, "require"=>0,
            "require_once"=>0, "return"=>0, "static"=>0, "switch"=>0,
            "throw"=>0, "trait"=>0, "try"=>0, "unset"=>0, "use"=>0, "var"=>0,
            "while"=>0, "xor"=>0, "int"=>0, "float"=>0, "bool"=>0, "string"=>0,
            "true"=>0, "false"=>0, "null"=>0, "void"=>0, "iterable"=>0
        );

        if (array_key_exists(strtolower($classname), $reserved_words)) {
            if ($file_proto->getPackage() === "google.protobuf") {
                return "GPB";
            } else {
                return "PB";
            }
        }

        return "";
    }

    public static function getLegacyClassNameWithoutPackage(
        $name,
        $file_proto)
    {
        $classname = implode('_', explode('.', $name));
        return static::getClassNamePrefix($classname, $file_proto) . $classname;
    }

    public static function getClassNameWithoutPackage(
        $name,
        $file_proto)
    {
        $parts = explode('.', $name);
        foreach ($parts as $i => $part) {
            $parts[$i] = static::getClassNamePrefix($parts[$i], $file_proto) . $parts[$i];
        }
        return implode('\\', $parts);
    }

    public static function getFullClassName(
        $proto,
        $containing,
        $file_proto,
        &$message_name_without_package,
        &$classname,
        &$legacy_classname,
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
            $fullname = $message_name_without_package;
        } else {
            $fullname = $package . "." . $message_name_without_package;
        }

        $class_name_without_package =
            static::getClassNameWithoutPackage($message_name_without_package, $file_proto);
        $legacy_class_name_without_package =
            static::getLegacyClassNameWithoutPackage(
                $message_name_without_package, $file_proto);

        $option = $file_proto->getOptions();
        if (!is_null($option) && $option->hasPhpNamespace()) {
            $namespace = $option->getPhpNamespace();
            if ($namespace !== "") {
                $classname = $namespace . "\\" . $class_name_without_package;
                $legacy_classname =
                    $namespace . "\\" . $legacy_class_name_without_package;
                return;
            } else {
                $classname = $class_name_without_package;
                $legacy_classname = $legacy_class_name_without_package;
                return;
            }
        }

        if ($package === "") {
            $classname = $class_name_without_package;
            $legacy_classname = $legacy_class_name_without_package;
        } else {
            $parts = array_map('ucwords', explode('.', $package));
            foreach ($parts as $i => $part) {
                $parts[$i] = self::getClassNamePrefix($part, $file_proto).$part;
            }
            $classname =
                implode('\\', $parts) .
                "\\".self::getClassNamePrefix($class_name_without_package,$file_proto).
                $class_name_without_package;
            $legacy_classname =
                implode('\\', array_map('ucwords', explode('.', $package))).
                "\\".$legacy_class_name_without_package;
        }
    }

    public static function combineInt32ToInt64($high, $low)
    {
        $isNeg = $high < 0;
        if ($isNeg) {
            $high = ~$high;
            $low = ~$low;
            $low++;
            if (!$low) {
                $high = (int) ($high + 1);
            }
        }
        $result = bcadd(bcmul($high, 4294967296), $low);
        if ($low < 0) {
            $result = bcadd($result, 4294967296);
        }
        if ($isNeg) {
          $result = bcsub(0, $result);
        }
        return $result;
    }

    public static function parseTimestamp($timestamp)
    {
        // prevent parsing timestamps containing with the non-existent year "0000"
        // DateTime::createFromFormat parses without failing but as a nonsensical date
        if (substr($timestamp, 0, 4) === "0000") {
            throw new \Exception("Year cannot be zero.");
        }
        // prevent parsing timestamps ending with a lowercase z
        if (substr($timestamp, -1, 1) === "z") {
            throw new \Exception("Timezone cannot be a lowercase z.");
        }

        $nanoseconds = 0;
        $periodIndex = strpos($timestamp, ".");
        if ($periodIndex !== false) {
            $nanosecondsLength = 0;
            // find the next non-numeric character in the timestamp to calculate
            // the length of the nanoseconds text
            for ($i = $periodIndex + 1, $length = strlen($timestamp); $i < $length; $i++) {
                if (!is_numeric($timestamp[$i])) {
                    $nanosecondsLength = $i - ($periodIndex + 1);
                    break;
                }
            }
            if ($nanosecondsLength % 3 !== 0) {
                throw new \Exception("Nanoseconds must be disible by 3.");
            }
            if ($nanosecondsLength > 9) {
                throw new \Exception("Nanoseconds must be in the range of 0 to 999,999,999 nanoseconds.");
            }
            if ($nanosecondsLength > 0) {
                $nanoseconds = substr($timestamp, $periodIndex + 1, $nanosecondsLength);
                $nanoseconds = intval($nanoseconds);

                // remove the nanoseconds and preceding period from the timestamp
                $date = substr($timestamp, 0, $periodIndex);
                $timezone = substr($timestamp, $periodIndex + $nanosecondsLength + 1);
                $timestamp = $date.$timezone;
            }
        }

        $date = \DateTime::createFromFormat(\DateTime::RFC3339, $timestamp, new \DateTimeZone("UTC"));
        if ($date === false) {
            throw new \Exception("Invalid RFC 3339 timestamp.");
        }

        $value = new \Google\Protobuf\Timestamp();
        $seconds = $date->format("U");
        $value->setSeconds($seconds);
        $value->setNanos($nanoseconds);
        return $value;
    }

    public static function formatTimestamp($value)
    {
        if (bccomp($value->getSeconds(), "253402300800") != -1) {
          throw new GPBDecodeException("Duration number too large.");
        }
        if (bccomp($value->getSeconds(), "-62135596801") != 1) {
          throw new GPBDecodeException("Duration number too small.");
        }
        $nanoseconds = static::getNanosecondsForTimestamp($value->getNanos());
        if (!empty($nanoseconds)) {
            $nanoseconds = ".".$nanoseconds;
        }
        $date = new \DateTime('@'.$value->getSeconds(), new \DateTimeZone("UTC"));
        return $date->format("Y-m-d\TH:i:s".$nanoseconds."\Z");
    }

    public static function parseDuration($value)
    {
        if (strlen($value) < 2 || substr($value, -1) !== "s") {
          throw new GPBDecodeException("Missing s after duration string");
        }
        $number = substr($value, 0, -1);
        if (bccomp($number, "315576000001") != -1) {
          throw new GPBDecodeException("Duration number too large.");
        }
        if (bccomp($number, "-315576000001") != 1) {
          throw new GPBDecodeException("Duration number too small.");
        }
        $pos = strrpos($number, ".");
        if ($pos !== false) {
            $seconds = substr($number, 0, $pos);
            if (bccomp($seconds, 0) < 0) {
                $nanos = bcmul("0" . substr($number, $pos), -1000000000);
            } else {
                $nanos = bcmul("0" . substr($number, $pos), 1000000000);
            }
        } else {
            $seconds = $number;
            $nanos = 0;
        }
        $duration = new Duration();
        $duration->setSeconds($seconds);
        $duration->setNanos($nanos);
        return $duration;
    }

    public static function formatDuration($value)
    {
        if (bccomp($value->getSeconds(), '315576000001') != -1) {
            throw new GPBDecodeException('Duration number too large.');
        }
        if (bccomp($value->getSeconds(), '-315576000001') != 1) {
            throw new GPBDecodeException('Duration number too small.');
        }

        $nanos = $value->getNanos();
        if ($nanos === 0) {
            return (string) $value->getSeconds();
        }

        if ($nanos % 1000000 === 0) {
            $digits = 3;
        } elseif ($nanos % 1000 === 0) {
            $digits = 6;
        } else {
            $digits = 9;
        }

        $nanos = bcdiv($nanos, '1000000000', $digits);
        return bcadd($value->getSeconds(), $nanos, $digits);
    }

    public static function parseFieldMask($paths_string)
    {
        $field_mask = new FieldMask();
        if (strlen($paths_string) === 0) {
            return $field_mask;
        }
        $path_strings = explode(",", $paths_string);
        $paths = $field_mask->getPaths();
        foreach($path_strings as &$path_string) {
            $field_strings = explode(".", $path_string);
            foreach($field_strings as &$field_string) {
                $field_string = camel2underscore($field_string);
            }
            $path_string = implode(".", $field_strings);
            $paths[] = $path_string;
        }
        return $field_mask;
    }

    public static function formatFieldMask($field_mask)
    {
        $converted_paths = [];
        foreach($field_mask->getPaths() as $path) {
            $fields = explode('.', $path);
            $converted_path = [];
            foreach ($fields as $field) {
                $segments = explode('_', $field);
                $start = true;
                $converted_segments = "";
                foreach($segments as $segment) {
                  if (!$start) {
                    $converted = ucfirst($segment);
                  } else {
                    $converted = $segment;
                    $start = false;
                  }
                  $converted_segments .= $converted;
                }
                $converted_path []= $converted_segments;
            }
            $converted_path = implode(".", $converted_path);
            $converted_paths []= $converted_path;
        }
        return implode(",", $converted_paths);
    }

    public static function getNanosecondsForTimestamp($nanoseconds)
    {
        if ($nanoseconds == 0) {
            return '';
        }
        if ($nanoseconds % static::NANOS_PER_MILLISECOND == 0) {
            return sprintf('%03d', $nanoseconds / static::NANOS_PER_MILLISECOND);
        }
        if ($nanoseconds % static::NANOS_PER_MICROSECOND == 0) {
            return sprintf('%06d', $nanoseconds / static::NANOS_PER_MICROSECOND);
        }
        return sprintf('%09d', $nanoseconds);
    }

    public static function hasSpecialJsonMapping($msg)
    {
        return is_a($msg, 'Google\Protobuf\Any')         ||
               is_a($msg, "Google\Protobuf\ListValue")   ||
               is_a($msg, "Google\Protobuf\Struct")      ||
               is_a($msg, "Google\Protobuf\Value")       ||
               is_a($msg, "Google\Protobuf\Duration")    ||
               is_a($msg, "Google\Protobuf\Timestamp")   ||
               is_a($msg, "Google\Protobuf\FieldMask")   ||
               static::hasJsonValue($msg);
    }

    public static function hasJsonValue($msg)
    {
        return is_a($msg, "Google\Protobuf\DoubleValue") ||
               is_a($msg, "Google\Protobuf\FloatValue")  ||
               is_a($msg, "Google\Protobuf\Int64Value")  ||
               is_a($msg, "Google\Protobuf\UInt64Value") ||
               is_a($msg, "Google\Protobuf\Int32Value")  ||
               is_a($msg, "Google\Protobuf\UInt32Value") ||
               is_a($msg, "Google\Protobuf\BoolValue")   ||
               is_a($msg, "Google\Protobuf\StringValue") ||
               is_a($msg, "Google\Protobuf\BytesValue");
    }
}
