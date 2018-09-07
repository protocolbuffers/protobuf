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

use Google\Protobuf\BoolValue;
use Google\Protobuf\BytesValue;
use Google\Protobuf\DoubleValue;
use Google\Protobuf\FloatValue;
use Google\Protobuf\Int32Value;
use Google\Protobuf\Int64Value;
use Google\Protobuf\StringValue;
use Google\Protobuf\UInt32Value;
use Google\Protobuf\UInt64Value;

/**
 * Utility functions for working with wrapper types.
 */
class GPBWrapperTypes
{
    private static $wrapperTypes = [
        DoubleValue::class,
        FloatValue::class,
        Int64Value::class,
        UInt64Value::class,
        Int32Value::class,
        UInt32Value::class,
        BoolValue::class,
        StringValue::class,
        BytesValue::class,
    ];

    /**
     * Determine if a fully qualified class name is a wrapper type.
     *
     * @param string $class the fully qualified class name
     * @return bool true if this is a wrapper type
     */
    public static function isWrapperType($class)
    {
        return in_array($class, self::$wrapperTypes);
    }

    /**
     * Tries to normalize $value into a provided protobuf wrapper type $class.
     * If $value is any type other than an object, we attempt to construct an
     * instance of $class and assign $value to it using the setValue method
     * shared by all wrapper types.
     *
     * @param mixed $value The value passed to the protobuf setter method
     * @param string $class The expected wrapper class name
     * @return Message An instance of $class
     * @throws \Exception If $value cannot be converted to a wrapper type
     */
    public static function normalizeToMessageType($value, $class)
    {
        if (is_null($value) || is_object($value)) {
            // This handles the case that $value is an instance of $class. We
            // choose not to do any more strict checking here, relying on the
            // existing type checking done by GPBUtil.
            return $value;
        } else {
            // Try to instantiate $class and set the value
            if (!self::isWrapperType($class)) {
                throw new \Exception("Expected wrapper type, got '$class'");
            }
            try {
                $msg = new $class;
                $msg->setValue($value);
                return $msg;
            } catch (\Exception $exception) {
                throw new \Exception(
                    "Error normalizing value to type '$class': " . $exception->getMessage(),
                    $exception->getCode(),
                    $exception
                );
            }
        }

    }
}
