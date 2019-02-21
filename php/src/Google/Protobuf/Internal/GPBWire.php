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

class GPBWire
{

    const TAG_TYPE_BITS = 3;

    const WIRETYPE_VARINT  = 0;
    const WIRETYPE_FIXED64 = 1;
    const WIRETYPE_LENGTH_DELIMITED = 2;
    const WIRETYPE_START_GROUP = 3;
    const WIRETYPE_END_GROUP = 4;
    const WIRETYPE_FIXED32 = 5;

    const UNKNOWN = 0;
    const NORMAL_FORMAT = 1;
    const PACKED_FORMAT = 2;

    public static function getTagFieldNumber($tag)
    {
        return ($tag >> self::TAG_TYPE_BITS) &
            (1 << ((PHP_INT_SIZE * 8) - self::TAG_TYPE_BITS)) - 1;
    }

    public static function getTagWireType($tag)
    {
        return $tag & 0x7;
    }

    public static function getWireType($type)
    {
        switch ($type) {
            case GPBType::FLOAT:
            case GPBType::FIXED32:
            case GPBType::SFIXED32:
                return self::WIRETYPE_FIXED32;
            case GPBType::DOUBLE:
            case GPBType::FIXED64:
            case GPBType::SFIXED64:
                return self::WIRETYPE_FIXED64;
            case GPBType::UINT32:
            case GPBType::UINT64:
            case GPBType::INT32:
            case GPBType::INT64:
            case GPBType::SINT32:
            case GPBType::SINT64:
            case GPBType::ENUM:
            case GPBType::BOOL:
                return self::WIRETYPE_VARINT;
            case GPBType::STRING:
            case GPBType::BYTES:
            case GPBType::MESSAGE:
                return self::WIRETYPE_LENGTH_DELIMITED;
            case GPBType::GROUP:
                user_error("Unsupported type.");
                return 0;
            default:
                user_error("Unsupported type.");
                return 0;
        }
    }

  // ZigZag Transform:  Encodes signed integers so that they can be effectively
  // used with varint encoding.
  //
  // varint operates on unsigned integers, encoding smaller numbers into fewer
  // bytes.  If you try to use it on a signed integer, it will treat this
  // number as a very large unsigned integer, which means that even small
  // signed numbers like -1 will take the maximum number of bytes (10) to
  // encode.  zigZagEncode() maps signed integers to unsigned in such a way
  // that those with a small absolute value will have smaller encoded values,
  // making them appropriate for encoding using varint.
  //
  // int32 ->     uint32
  // -------------------------
  //           0 ->          0
  //          -1 ->          1
  //           1 ->          2
  //          -2 ->          3
  //         ... ->        ...
  //  2147483647 -> 4294967294
  // -2147483648 -> 4294967295
  //
  //        >> encode >>
  //        << decode <<
  public static function zigZagEncode32($int32)
  {
      if (PHP_INT_SIZE == 8) {
          $trim_int32 = $int32 & 0xFFFFFFFF;
          return (($trim_int32 << 1) ^ ($int32 << 32 >> 63)) & 0xFFFFFFFF;
      } else {
          return ($int32 << 1) ^ ($int32 >> 31);
      }
  }

    public static function zigZagDecode32($uint32)
    {
        // Fill high 32 bits.
        if (PHP_INT_SIZE === 8) {
            $uint32 |= ($uint32 & 0xFFFFFFFF);
        }

        $int32 = (($uint32 >> 1) & 0x7FFFFFFF) ^ (-($uint32 & 1));

        return $int32;
    }

    public static function zigZagEncode64($int64)
    {
        if (PHP_INT_SIZE == 4) {
            if (bccomp($int64, 0) >= 0) {
                return bcmul($int64, 2);
            } else {
                return bcsub(bcmul(bcsub(0, $int64), 2), 1);
            }
        } else {
            return ($int64 << 1) ^ ($int64 >> 63);
        }
    }

    public static function zigZagDecode64($uint64)
    {
        if (PHP_INT_SIZE == 4) {
            if (bcmod($uint64, 2) == 0) {
                return bcdiv($uint64, 2, 0);
            } else {
                return bcsub(0, bcdiv(bcadd($uint64, 1), 2, 0));
            }
        } else {
            return (($uint64 >> 1) & 0x7FFFFFFFFFFFFFFF) ^ (-($uint64 & 1));
        }
    }

    public static function readInt32(&$input, &$value)
    {
        return $input->readVarint32($value);
    }

    public static function readInt64(&$input, &$value)
    {
        $success = $input->readVarint64($value);
        if (PHP_INT_SIZE == 4 && bccomp($value, "9223372036854775807") > 0) {
            $value = bcsub($value, "18446744073709551616");
        }
        return $success;
    }

    public static function readUint32(&$input, &$value)
    {
        return self::readInt32($input, $value);
    }

    public static function readUint64(&$input, &$value)
    {
        return self::readInt64($input, $value);
    }

    public static function readSint32(&$input, &$value)
    {
        if (!$input->readVarint32($value)) {
            return false;
        }
        $value = GPBWire::zigZagDecode32($value);
        return true;
    }

    public static function readSint64(&$input, &$value)
    {
        if (!$input->readVarint64($value)) {
            return false;
        }
        $value = GPBWire::zigZagDecode64($value);
        return true;
    }

    public static function readFixed32(&$input, &$value)
    {
        return $input->readLittleEndian32($value);
    }

    public static function readFixed64(&$input, &$value)
    {
        return $input->readLittleEndian64($value);
    }

    public static function readSfixed32(&$input, &$value)
    {
        if (!self::readFixed32($input, $value)) {
            return false;
        }
        if (PHP_INT_SIZE === 8) {
            $value |= (-($value >> 31) << 32);
        }
        return true;
    }

    public static function readSfixed64(&$input, &$value)
    {
        $success = $input->readLittleEndian64($value);
        if (PHP_INT_SIZE == 4 && bccomp($value, "9223372036854775807") > 0) {
            $value = bcsub($value, "18446744073709551616");
        }
        return $success;
    }

    public static function readFloat(&$input, &$value)
    {
        $data = null;
        if (!$input->readRaw(4, $data)) {
            return false;
        }
        $value = unpack('f', $data)[1];
        return true;
    }

    public static function readDouble(&$input, &$value)
    {
        $data = null;
        if (!$input->readRaw(8, $data)) {
            return false;
        }
        $value = unpack('d', $data)[1];
        return true;
    }

    public static function readBool(&$input, &$value)
    {
        if (!$input->readVarint64($value)) {
            return false;
        }
        if ($value == 0) {
            $value = false;
        } else {
            $value = true;
        }
        return true;
    }

    public static function readString(&$input, &$value)
    {
        $length = 0;
        return $input->readVarintSizeAsInt($length) && $input->readRaw($length, $value);
    }

    public static function readMessage(&$input, &$message)
    {
        $length = 0;
        if (!$input->readVarintSizeAsInt($length)) {
            return false;
        }
        $old_limit = 0;
        $recursion_limit = 0;
        $input->incrementRecursionDepthAndPushLimit(
            $length,
            $old_limit,
            $recursion_limit);
        if ($recursion_limit < 0 || !$message->parseFromStream($input)) {
            return false;
        }
        return $input->decrementRecursionDepthAndPopLimit($old_limit);
    }

    public static function writeTag(&$output, $tag)
    {
        return $output->writeTag($tag);
    }

    public static function writeInt32(&$output, $value)
    {
        return $output->writeVarint32($value, false);
    }

    public static function writeInt64(&$output, $value)
    {
        return $output->writeVarint64($value);
    }

    public static function writeUint32(&$output, $value)
    {
        return $output->writeVarint32($value, true);
    }

    public static function writeUint64(&$output, $value)
    {
        return $output->writeVarint64($value);
    }

    public static function writeSint32(&$output, $value)
    {
        $value = GPBWire::zigZagEncode32($value);
        return $output->writeVarint32($value, true);
    }

    public static function writeSint64(&$output, $value)
    {
        $value = GPBWire::zigZagEncode64($value);
        return $output->writeVarint64($value);
    }

    public static function writeFixed32(&$output, $value)
    {
        return $output->writeLittleEndian32($value);
    }

    public static function writeFixed64(&$output, $value)
    {
        return $output->writeLittleEndian64($value);
    }

    public static function writeSfixed32(&$output, $value)
    {
        return $output->writeLittleEndian32($value);
    }

    public static function writeSfixed64(&$output, $value)
    {
        return $output->writeLittleEndian64($value);
    }

    public static function writeBool(&$output, $value)
    {
        if ($value) {
            return $output->writeVarint32(1, true);
        } else {
            return $output->writeVarint32(0, true);
        }
    }

    public static function writeFloat(&$output, $value)
    {
        $data = pack("f", $value);
        return $output->writeRaw($data, 4);
    }

    public static function writeDouble(&$output, $value)
    {
        $data = pack("d", $value);
        return $output->writeRaw($data, 8);
    }

    public static function writeString(&$output, $value)
    {
        return self::writeBytes($output, $value);
    }

    public static function writeBytes(&$output, $value)
    {
        $size = strlen($value);
        if (!$output->writeVarint32($size, true)) {
            return false;
        }
        return $output->writeRaw($value, $size);
    }

    public static function writeMessage(&$output, $value)
    {
        $size = $value->byteSize();
        if (!$output->writeVarint32($size, true)) {
            return false;
        }
        return $value->serializeToStream($output);
    }

    public static function makeTag($number, $type)
    {
        return ($number << 3) | self::getWireType($type);
    }

    public static function tagSize($field)
    {
        $tag = self::makeTag($field->getNumber(), $field->getType());
        return self::varint32Size($tag);
    }

    public static function varint32Size($value, $sign_extended = false)
    {
        if ($value < 0) {
            if ($sign_extended) {
                return 10;
            } else {
                return 5;
            }
        }
        if ($value < (1 <<  7)) {
            return 1;
        }
        if ($value < (1 << 14)) {
            return 2;
        }
        if ($value < (1 << 21)) {
            return 3;
        }
        if ($value < (1 << 28)) {
            return 4;
        }
        return 5;
    }

    public static function sint32Size($value)
    {
        $value = self::zigZagEncode32($value);
        return self::varint32Size($value);
    }

    public static function sint64Size($value)
    {
        $value = self::zigZagEncode64($value);
        return self::varint64Size($value);
    }

    public static function varint64Size($value)
    {
        if (PHP_INT_SIZE == 4) {
            if (bccomp($value, 0) < 0 ||
                bccomp($value, "9223372036854775807") > 0) {
                return 10;
            }
            if (bccomp($value, 1 << 7) < 0) {
                return 1;
            }
            if (bccomp($value, 1 << 14) < 0) {
                return 2;
            }
            if (bccomp($value, 1 << 21) < 0) {
                return 3;
            }
            if (bccomp($value, 1 << 28) < 0) {
                return 4;
            }
            if (bccomp($value, '34359738368') < 0) {
                return 5;
            }
            if (bccomp($value, '4398046511104') < 0) {
                return 6;
            }
            if (bccomp($value, '562949953421312') < 0) {
                return 7;
            }
            if (bccomp($value, '72057594037927936') < 0) {
                return 8;
            }
            return 9;
        } else {
            if ($value < 0) {
                return 10;
            }
            if ($value < (1 <<  7)) {
                return 1;
            }
            if ($value < (1 << 14)) {
                return 2;
            }
            if ($value < (1 << 21)) {
                return 3;
            }
            if ($value < (1 << 28)) {
                return 4;
            }
            if ($value < (1 << 35)) {
                return 5;
            }
            if ($value < (1 << 42)) {
                return 6;
            }
            if ($value < (1 << 49)) {
                return 7;
            }
            if ($value < (1 << 56)) {
                return 8;
            }
            return 9;
        }
    }

    public static function serializeFieldToStream(
        $value,
        $field,
        $need_tag,
        &$output)
    {
        if ($need_tag) {
            if (!GPBWire::writeTag(
                $output,
                self::makeTag(
                    $field->getNumber(),
                    $field->getType()))) {
                return false;
            }
        }
        switch ($field->getType()) {
            case GPBType::DOUBLE:
                if (!GPBWire::writeDouble($output, $value)) {
                    return false;
                }
                break;
            case GPBType::FLOAT:
                if (!GPBWire::writeFloat($output, $value)) {
                    return false;
                }
                break;
            case GPBType::INT64:
                if (!GPBWire::writeInt64($output, $value)) {
                    return false;
                }
                break;
            case GPBType::UINT64:
                if (!GPBWire::writeUint64($output, $value)) {
                    return false;
                }
                break;
            case GPBType::INT32:
                if (!GPBWire::writeInt32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::FIXED32:
                if (!GPBWire::writeFixed32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::FIXED64:
                if (!GPBWire::writeFixed64($output, $value)) {
                    return false;
                }
                break;
            case GPBType::BOOL:
                if (!GPBWire::writeBool($output, $value)) {
                    return false;
                }
                break;
            case GPBType::STRING:
                if (!GPBWire::writeString($output, $value)) {
                    return false;
                }
                break;
            //    case GPBType::GROUP:
            //      echo "GROUP\xA";
            //      trigger_error("Not implemented.", E_ERROR);
            //      break;
            case GPBType::MESSAGE:
                if (!GPBWire::writeMessage($output, $value)) {
                    return false;
                }
                break;
            case GPBType::BYTES:
                if (!GPBWire::writeBytes($output, $value)) {
                    return false;
                }
                break;
            case GPBType::UINT32:
                if (PHP_INT_SIZE === 8 && $value < 0) {
                    $value += 4294967296;
                }
                if (!GPBWire::writeUint32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::ENUM:
                if (!GPBWire::writeInt32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::SFIXED32:
                if (!GPBWire::writeSfixed32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::SFIXED64:
                if (!GPBWire::writeSfixed64($output, $value)) {
                    return false;
                }
                break;
            case GPBType::SINT32:
                if (!GPBWire::writeSint32($output, $value)) {
                    return false;
                }
                break;
            case GPBType::SINT64:
                if (!GPBWire::writeSint64($output, $value)) {
                    return false;
                }
                break;
            default:
                user_error("Unsupported type.");
                return false;
        }

        return true;
    }
}
