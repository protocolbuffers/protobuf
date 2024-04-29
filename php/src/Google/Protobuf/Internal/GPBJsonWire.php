<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

class GPBJsonWire
{

    public static function serializeFieldToStream(
        $value,
        $field,
        &$output, $has_field_name = true)
    {
        if ($has_field_name) {
            $output->writeRaw("\"", 1);
            $field_name = GPBJsonWire::formatFieldName($field);
            $output->writeRaw($field_name, strlen($field_name));
            $output->writeRaw("\":", 2);
        }
        return static::serializeFieldValueToStream(
            $value,
            $field,
            $output,
            !$has_field_name);
    }

    public static function serializeFieldValueToStream(
        $values,
        $field,
        &$output,
        $is_well_known = false)
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
                    $output,
                    $is_well_known)) {
                    return false;
                }
                if ($additional_quote) {
                    $output->writeRaw("\"", 1);
                }
                $output->writeRaw(":", 1);
                if (!static::serializeSingularFieldValueToStream(
                    $value,
                    $value_field,
                    $output,
                    $is_well_known)) {
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
                    $output,
                    $is_well_known)) {
                    return false;
                }
            }
            $output->writeRaw("]", 1);
            return true;
        } else {
            return static::serializeSingularFieldValueToStream(
                $values,
                $field,
                $output,
                $is_well_known);
        }
    }

    private static function serializeSingularFieldValueToStream(
        $value,
        $field,
        &$output, $is_well_known = false)
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
                if ($enum_desc->getClass() === "Google\Protobuf\NullValue") {
                    $output->writeRaw("null", 4);
                    break;
                }
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
                $bytes_value = base64_encode($value);
                $output->writeRaw("\"", 1);
                $output->writeRaw($bytes_value, strlen($bytes_value));
                $output->writeRaw("\"", 1);
                break;
            case GPBType::STRING:
                $value = json_encode($value, JSON_UNESCAPED_UNICODE);
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
