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

class GPBJsonWire
{

    public static function serializeFieldToStream(
        $value,
        $field,
        &$output)
    {
        $output->writeRaw("\"", 1);
        $field_name = GPBJsonWire::formatFieldName($field);
        $output->writeRaw($field_name, strlen($field_name));
        $output->writeRaw("\":", 2);
        return static::serializeFieldValueToStream($value, $field, $output);
    }

    private static function serializeFieldValueToStream(
        $values,
        $field,
        &$output)
    {
        if ($field->isMap()) {
            $output->writeRaw("{", 1);
            $first = true;
            $map_entry = $field->getMessageType();
            $key_field = $map_entry->getFieldByNumber(1);
            $value_field = $map_entry->getFieldByNumber(2);

            switch ($key_field->getType()) {
            case GPBType::STRING:
            case GPBType::SFIXED64:
            case GPBType::INT64:
            case GPBType::SINT64:
            case GPBType::FIXED64:
            case GPBType::UINT64:
                $additional_quote = false;
                break;
            default:
                $additional_quote = true;
            }

            foreach ($values as $key => $value) {
                if ($first) {
                    $first = false;
                } else {
                    $output->writeRaw(",", 1);
                }
                if ($additional_quote) {
                    $output->writeRaw("\"", 1);
                }
                if (!static::serializeSingularFieldValueToStream(
                    $key,
                    $key_field,
                    $output)) {
                    return false;
                }
                if ($additional_quote) {
                    $output->writeRaw("\"", 1);
                }
                $output->writeRaw(":", 1);
                if (!static::serializeSingularFieldValueToStream(
                    $value,
                    $value_field,
                    $output)) {
                    return false;
                }
            }
            $output->writeRaw("}", 1);
            return true;
        } elseif ($field->isRepeated()) {
            $output->writeRaw("[", 1);
            $first = true;
            foreach ($values as $value) {
                if ($first) {
                    $first = false;
                } else {
                    $output->writeRaw(",", 1);
                }
                if (!static::serializeSingularFieldValueToStream(
                    $value,
                    $field,
                    $output)) {
                    return false;
                }
            }
            $output->writeRaw("]", 1);
            return true;
        } else {
            return static::serializeSingularFieldValueToStream(
                $values,
                $field,
                $output);
        }
    }

    private static function serializeSingularFieldValueToStream(
        $value,
        $field,
        &$output)
    {
        switch ($field->getType()) {
            case GPBType::SFIXED32:
            case GPBType::SINT32:
            case GPBType::INT32:
                $str_value = strval($value);
                $output->writeRaw($str_value, strlen($str_value));
                break;
            case GPBType::FIXED32:
            case GPBType::UINT32:
                if ($value < 0) {
                    $value = bcadd($value, "4294967296");
                }
                $str_value = strval($value);
                $output->writeRaw($str_value, strlen($str_value));
                break;
            case GPBType::FIXED64:
            case GPBType::UINT64:
                if ($value < 0) {
                    $value = bcadd($value, "18446744073709551616");
                }
                // Intentional fall through.
            case GPBType::SFIXED64:
            case GPBType::INT64:
            case GPBType::SINT64:
                $output->writeRaw("\"", 1);
                $str_value = strval($value);
                $output->writeRaw($str_value, strlen($str_value));
                $output->writeRaw("\"", 1);
                break;
            case GPBType::FLOAT:
                if (is_nan($value)) {
                    $str_value = "\"NaN\"";
                } elseif ($value === INF) {
                    $str_value = "\"Infinity\"";
                } elseif ($value === -INF) {
                    $str_value = "\"-Infinity\"";
                } else {
                    $str_value = sprintf("%.8g", $value);
                }
                $output->writeRaw($str_value, strlen($str_value));
                break;
            case GPBType::DOUBLE:
                if (is_nan($value)) {
                    $str_value = "\"NaN\"";
                } elseif ($value === INF) {
                    $str_value = "\"Infinity\"";
                } elseif ($value === -INF) {
                    $str_value = "\"-Infinity\"";
                } else {
                    $str_value = sprintf("%.17g", $value);
                }
                $output->writeRaw($str_value, strlen($str_value));
                break;
            case GPBType::ENUM:
                $enum_desc = $field->getEnumType();
                $enum_value_desc = $enum_desc->getValueByNumber($value);
                if (!is_null($enum_value_desc)) {
                    $str_value = $enum_value_desc->getName();
                    $output->writeRaw("\"", 1);
                    $output->writeRaw($str_value, strlen($str_value));
                    $output->writeRaw("\"", 1);
                } else {
                    $str_value = strval($value);
                    $output->writeRaw($str_value, strlen($str_value));
                }
                break;
            case GPBType::BOOL:
                if ($value) {
                    $output->writeRaw("true", 4);
                } else {
                    $output->writeRaw("false", 5);
                }
                break;
            case GPBType::BYTES:
                $value = base64_encode($value);
            case GPBType::STRING:
                $value = json_encode($value);
                $output->writeRaw($value, strlen($value));
                break;
            //    case GPBType::GROUP:
            //      echo "GROUP\xA";
            //      trigger_error("Not implemented.", E_ERROR);
            //      break;
            case GPBType::MESSAGE:
                $value->serializeToJsonStream($output);
                break;
            default:
                user_error("Unsupported type.");
                return false;
        }
        return true;
    }

    private static function formatFieldName($field)
    {
        return $field->getJsonName();
    }

    // Used for escaping control chars in strings.
    private static $k_control_char_limit = 0x20;

    private static function jsonNiceEscape($c)
    {
      switch ($c) {
          case '"':  return "\\\"";
          case '\\': return "\\\\";
          case '/': return "\\/";
          case '\b': return "\\b";
          case '\f': return "\\f";
          case '\n': return "\\n";
          case '\r': return "\\r";
          case '\t': return "\\t";
          default:   return NULL;
      }
    }

    private static function isJsonEscaped($c)
    {
        // See RFC 4627.
        return $c < chr($k_control_char_limit) || $c === "\"" || $c === "\\";
    }

    public static function escapedJson($value)
    {
        $escaped_value = "";
        $unescaped_run = "";
        for ($i = 0; $i < strlen($value); $i++) {
            $c = $value[$i];
            // Handle escaping.
            if (static::isJsonEscaped($c)) {
                // Use a "nice" escape, like \n, if one exists for this
                // character.
                $escape = static::jsonNiceEscape($c);
                if (is_null($escape)) {
                    $escape = "\\u00" . bin2hex($c);
                }
                if ($unescaped_run !== "") {
                    $escaped_value .= $unescaped_run;
                    $unescaped_run = "";
                }
                $escaped_value .= $escape;
            } else {
              if ($unescaped_run === "") {
                $unescaped_run .= $c;
              }
            }
        }
        $escaped_value .= $unescaped_run;
        return $escaped_value;
    }

}
