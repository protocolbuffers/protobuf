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

/**
 * Defines Message, the parent class extended by all protocol message classes.
 */

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\InputStream;
use Google\Protobuf\Internal\OutputStream;
use Google\Protobuf\Internal\DescriptorPool;
use Google\Protobuf\Internal\GPBLabel;
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\GPBWire;
use Google\Protobuf\Internal\MapEntry;
use Google\Protobuf\Internal\RepeatedField;

/**
 * Parent class of all proto messages. Users should not instantiate this class
 * or extend this class or its child classes by their own.  See the comment of
 * specific functions for more details.
 */
class Message
{

    /**
     * @ignore
     */
    private $desc;

    /**
     * @ignore
     */
    public function __construct($desc = NULL)
    {
        // MapEntry message is shared by all types of map fields, whose
        // descriptors are different from each other. Thus, we cannot find a
        // specific descriptor from the descriptor pool.
        if (get_class($this) === 'Google\Protobuf\Internal\MapEntry') {
            $this->desc = $desc;
            return;
        }
        $pool = DescriptorPool::getGeneratedPool();
        $this->desc = $pool->getDescriptorByClassName(get_class($this));
        foreach ($this->desc->getField() as $field) {
            $setter = $field->getSetter();
            if ($field->isMap()) {
                $message_type = $field->getMessageType();
                $key_field = $message_type->getFieldByNumber(1);
                $value_field = $message_type->getFieldByNumber(2);
                switch ($value_field->getType()) {
                    case GPBType::MESSAGE:
                    case GPBType::GROUP:
                        $map_field = new MapField(
                            $key_field->getType(),
                            $value_field->getType(),
                            $value_field->getMessageType()->getClass());
                        $this->$setter($map_field);
                        break;
                    case GPBType::ENUM:
                        $map_field = new MapField(
                            $key_field->getType(),
                            $value_field->getType(),
                            $value_field->getEnumType()->getClass());
                        $this->$setter($map_field);
                        break;
                    default:
                        $map_field = new MapField(
                            $key_field->getType(),
                            $value_field->getType());
                        $this->$setter($map_field);
                        break;
                }
            } else if ($field->getLabel() === GPBLabel::REPEATED) {
                switch ($field->getType()) {
                    case GPBType::MESSAGE:
                    case GPBType::GROUP:
                        $repeated_field = new RepeatedField(
                            $field->getType(),
                            $field->getMessageType()->getClass());
                        $this->$setter($repeated_field);
                        break;
                    case GPBType::ENUM:
                        $repeated_field = new RepeatedField(
                            $field->getType(),
                            $field->getEnumType()->getClass());
                        $this->$setter($repeated_field);
                        break;
                    default:
                        $repeated_field = new RepeatedField($field->getType());
                        $this->$setter($repeated_field);
                        break;
                }
            } else if ($field->getOneofIndex() !== -1) {
                $oneof = $this->desc->getOneofDecl()[$field->getOneofIndex()];
                $oneof_name = $oneof->getName();
                $this->$oneof_name = new OneofField($oneof);
            }
        }
    }

    protected function readOneof($number)
    {
        $field = $this->desc->getFieldByNumber($number);
        $oneof = $this->desc->getOneofDecl()[$field->getOneofIndex()];
        $oneof_name = $oneof->getName();
        $oneof_field = $this->$oneof_name;
        if ($number === $oneof_field->getNumber()) {
            return $oneof_field->getValue();
        } else {
            return $this->defaultValue($field);
        }
    }

    protected function writeOneof($number, $value)
    {
        $field = $this->desc->getFieldByNumber($number);
        $oneof = $this->desc->getOneofDecl()[$field->getOneofIndex()];
        $oneof_name = $oneof->getName();
        $oneof_field = $this->$oneof_name;
        $oneof_field->setValue($value);
        $oneof_field->setFieldName($field->getName());
        $oneof_field->setNumber($number);
    }

    /**
     * @ignore
     */
    private function defaultValue($field)
    {
        $value = null;

        switch ($field->getType()) {
            case GPBType::DOUBLE:
            case GPBType::FLOAT:
                return 0.0;
            case GPBType::UINT32:
            case GPBType::UINT64:
            case GPBType::INT32:
            case GPBType::INT64:
            case GPBType::FIXED32:
            case GPBType::FIXED64:
            case GPBType::SFIXED32:
            case GPBType::SFIXED64:
            case GPBType::SINT32:
            case GPBType::SINT64:
            case GPBType::ENUM:
                return 0;
            case GPBType::BOOL:
                return false;
            case GPBType::STRING:
            case GPBType::BYTES:
                return "";
            case GPBType::GROUP:
            case GPBType::MESSAGE:
                return null;
            default:
                user_error("Unsupported type.");
                return false;
        }
    }

    /**
     * @ignore
     */
    private static function parseFieldFromStreamNoTag($input, $field, &$value)
    {
        switch ($field->getType()) {
            case GPBType::DOUBLE:
                if (!GPBWire::readDouble($input, $value)) {
                    return false;
                }
                break;
            case GPBType::FLOAT:
                if (!GPBWire::readFloat($input, $value)) {
                    return false;
                }
                break;
            case GPBType::INT64:
                if (!GPBWire::readInt64($input, $value)) {
                    return false;
                }
                $value = $value->toInteger();
                break;
            case GPBType::UINT64:
                if (!GPBWire::readUint64($input, $value)) {
                    return false;
                }
                $value = $value->toInteger();
                break;
            case GPBType::INT32:
                if (!GPBWire::readInt32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::FIXED64:
                if (!GPBWire::readFixed64($input, $value)) {
                    return false;
                }
                $value = $value->toInteger();
                break;
            case GPBType::FIXED32:
                if (!GPBWire::readFixed32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::BOOL:
                if (!GPBWire::readBool($input, $value)) {
                    return false;
                }
                break;
            case GPBType::STRING:
                // TODO(teboring): Add utf-8 check.
                if (!GPBWire::readString($input, $value)) {
                    return false;
                }
                break;
            case GPBType::GROUP:
                echo "GROUP\xA";
                trigger_error("Not implemented.", E_ERROR);
                break;
            case GPBType::MESSAGE:
                if ($field->isMap()) {
                    $value = new MapEntry($field->getMessageType());
                } else {
                    $klass = $field->getMessageType()->getClass();
                    $value = new $klass;
                }
                if (!GPBWire::readMessage($input, $value)) {
                    return false;
                }
                break;
            case GPBType::BYTES:
                if (!GPBWire::readString($input, $value)) {
                    return false;
                }
                break;
            case GPBType::UINT32:
                if (!GPBWire::readUint32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::ENUM:
                // TODO(teboring): Check unknown enum value.
                if (!GPBWire::readInt32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::SFIXED32:
                if (!GPBWire::readSfixed32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::SFIXED64:
                if (!GPBWire::readSfixed64($input, $value)) {
                    return false;
                }
                $value = $value->toInteger();
                break;
            case GPBType::SINT32:
                if (!GPBWire::readSint32($input, $value)) {
                    return false;
                }
                break;
            case GPBType::SINT64:
                if (!GPBWire::readSint64($input, $value)) {
                    return false;
                }
                $value = $value->toInteger();
                break;
            default:
                user_error("Unsupported type.");
                return false;
        }
        return true;
    }

    /**
     * @ignore
     */
    private function parseFieldFromStream($tag, $input, $field)
    {
        $value = null;
        $field_type = $field->getType();

        $value_format = GPBWire::UNKNOWN;
        if (GPBWire::getTagWireType($tag) ===
            GPBWire::getWireType($field_type)) {
            $value_format = GPBWire::NORMAL_FORMAT;
        } elseif ($field->isPackable() &&
            GPBWire::getTagWireType($tag) ===
            GPBWire::WIRETYPE_LENGTH_DELIMITED) {
            $value_format = GPBWire::PACKED_FORMAT;
        }

        if ($value_format === GPBWire::NORMAL_FORMAT) {
            if (!self::parseFieldFromStreamNoTag($input, $field, $value)) {
                return false;
            }
        } elseif ($value_format === GPBWire::PACKED_FORMAT) {
            $length = 0;
            if (!GPBWire::readInt32($input, $length)) {
                return false;
            }
            $limit = $input->pushLimit($length);
            $getter = $field->getGetter();
            while ($input->bytesUntilLimit() > 0) {
                if (!self::parseFieldFromStreamNoTag($input, $field, $value)) {
                    return false;
                }
                $this->$getter()[] = $value;
            }
            $input->popLimit($limit);
            return true;
        } else {
            return false;
        }

        if ($field->isMap()) {
            $getter = $field->getGetter();
            $this->$getter()[$value->getKey()] = $value->getValue();
        } else if ($field->isRepeated()) {
            $getter = $field->getGetter();
            $this->$getter()[] = $value;
        } else {
            $setter = $field->getSetter();
            $this->$setter($value);
        }

        return true;
    }

    /**
     * Parses a protocol buffer contained in a string.
     *
     * This function takes a string in the (non-human-readable) binary wire
     * format, matching the encoding output by encode().
     *
     * @param string $data Binary protobuf data.
     * @return bool Return true on success.
     */
    public function decode($data)
    {
        $input = new InputStream($data);
        $this->parseFromStream($input);
    }

    /**
     * @ignore
     */
    public function parseFromStream($input)
    {
        while (true) {
            $tag = $input->readTag();
            // End of input.  This is a valid place to end, so return true.
            if ($tag === 0) {
                return true;
            }

            $number = GPBWire::getTagFieldNumber($tag);
            $field = $this->desc->getFieldByNumber($number);

            if (!$this->parseFieldFromStream($tag, $input, $field)) {
                return false;
            }
        }
    }

    /**
     * @ignore
     */
    private function serializeSingularFieldToStream($field, &$output)
    {
        if (!$this->existField($field)) {
            return true;
        }
        $getter = $field->getGetter();
        $value = $this->$getter();
        if (!GPBWire::serializeFieldToStream($value, $field, true, $output)) {
            return false;
        }
        return true;
    }

    /**
     * @ignore
     */
    private function serializeRepeatedFieldToStream($field, &$output)
    {
        $getter = $field->getGetter();
        $values = $this->$getter();
        $count = count($values);
        if ($count === 0) {
            return true;
        }

        $packed = $field->getPacked();
        if ($packed) {
            if (!GPBWire::writeTag(
                $output,
                GPBWire::makeTag($field->getNumber(), GPBType::STRING))) {
                return false;
            }
            $size = 0;
            foreach ($values as $value) {
                $size += $this->fieldDataOnlyByteSize($field, $value);
            }
            if (!$output->writeVarint32($size)) {
                return false;
            }
        }

        foreach ($values as $value) {
            if (!GPBWire::serializeFieldToStream(
                $value,
                $field,
                !$packed,
                $output)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @ignore
     */
    private function serializeMapFieldToStream($field, $output)
    {
        $getter = $field->getGetter();
        $values = $this->$getter();
        $count = count($values);
        if ($count === 0) {
            return true;
        }

        foreach ($values as $key => $value) {
            $map_entry = new MapEntry($field->getMessageType());
            $map_entry->setKey($key);
            $map_entry->setValue($value);
            if (!GPBWire::serializeFieldToStream(
                $map_entry,
                $field,
                true,
                $output)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @ignore
     */
    private function serializeFieldToStream(&$output, $field)
    {
        if ($field->isMap()) {
            return $this->serializeMapFieldToStream($field, $output);
        } elseif ($field->isRepeated()) {
            return $this->serializeRepeatedFieldToStream($field, $output);
        } else {
            return $this->serializeSingularFieldToStream($field, $output);
        }
    }

    /**
     * @ignore
     */
    public function serializeToStream(&$output)
    {
        $fields = $this->desc->getField();
        foreach ($fields as $field) {
            if (!$this->serializeFieldToStream($output, $field)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Serialize the message to string.
     * @return string Serialized binary protobuf data.
     */
    public function encode()
    {
        $output = new OutputStream($this->byteSize());
        $this->serializeToStream($output);
        return $output->getData();
    }

    /**
     * @ignore
     */
    private function existField($field)
    {
        $getter = $field->getGetter();
        $value = $this->$getter();
        return $value !== $this->defaultValue($field);
    }

    /**
     * @ignore
     */
    private function repeatedFieldDataOnlyByteSize($field)
    {
        $size = 0;

        $getter = $field->getGetter();
        $values = $this->$getter();
        $count = count($values);
        if ($count !== 0) {
            $size += $count * GPBWire::tagSize($field);
            foreach ($values as $value) {
                $size += $this->singularFieldDataOnlyByteSize($field);
            }
        }
    }

    /**
     * @ignore
     */
    private function fieldDataOnlyByteSize($field, $value)
    {
        $size = 0;

        switch ($field->getType()) {
            case GPBType::BOOL:
                $size += 1;
                break;
            case GPBType::FLOAT:
            case GPBType::FIXED32:
            case GPBType::SFIXED32:
                $size += 4;
                break;
            case GPBType::DOUBLE:
            case GPBType::FIXED64:
            case GPBType::SFIXED64:
                $size += 8;
                break;
            case GPBType::UINT32:
            case GPBType::INT32:
            case GPBType::ENUM:
                $size += GPBWire::varint32Size($value);
                break;
            case GPBType::UINT64:
            case GPBType::INT64:
                $size += GPBWire::varint64Size($value);
                break;
            case GPBType::SINT32:
                $size += GPBWire::sint32Size($value);
                break;
            case GPBType::SINT64:
                $size += GPBWire::sint64Size($value);
                break;
            case GPBType::STRING:
            case GPBType::BYTES:
                $size += strlen($value);
                $size += GPBWire::varint32Size($size);
                break;
            case GPBType::MESSAGE:
                $size += $value->byteSize();
                $size += GPBWire::varint32Size($size);
                break;
            case GPBType::GROUP:
                // TODO(teboring): Add support.
                user_error("Unsupported type.");
                break;
            default:
                user_error("Unsupported type.");
                return 0;
        }

        return $size;
    }

    /**
     * @ignore
     */
    private function fieldByteSize($field)
    {
        $size = 0;
        if ($field->isMap()) {
            $getter = $field->getGetter();
            $values = $this->$getter();
            $count = count($values);
            if ($count !== 0) {
                $size += $count * GPBWire::tagSize($field);
                $message_type = $field->getMessageType();
                $key_field = $message_type->getFieldByNumber(1);
                $value_field = $message_type->getFieldByNumber(2);
                foreach ($values as $key => $value) {
                    $data_size = 0;
                    $data_size += $this->fieldDataOnlyByteSize($key_field, $key);
                    $data_size += $this->fieldDataOnlyByteSize(
                        $value_field,
                        $value);
                    $data_size += GPBWire::tagSize($key_field);
                    $data_size += GPBWire::tagSize($value_field);
                    $size += GPBWire::varint32Size($data_size) + $data_size;
                }
            }
        } elseif ($field->isRepeated()) {
            $getter = $field->getGetter();
            $values = $this->$getter();
            $count = count($values);
            if ($count !== 0) {
                if ($field->getPacked()) {
                    $data_size = 0;
                    foreach ($values as $value) {
                        $data_size += $this->fieldDataOnlyByteSize($field, $value);
                    }
                    $size += GPBWire::tagSize($field);
                    $size += GPBWire::varint32Size($data_size);
                    $size += $data_size;
                } else {
                    $size += $count * GPBWire::tagSize($field);
                    foreach ($values as $value) {
                        $size += $this->fieldDataOnlyByteSize($field, $value);
                    }
                }
            }
        } elseif ($this->existField($field)) {
            $size += GPBWire::tagSize($field);
            $getter = $field->getGetter();
            $value = $this->$getter();
            $size += $this->fieldDataOnlyByteSize($field, $value);
        }
        return $size;
    }

    /**
     * @ignore
     */
    public function byteSize()
    {
        $size = 0;

        $fields = $this->desc->getField();
        foreach ($fields as $field) {
            $size += $this->fieldByteSize($field);
        }
        return $size;
    }
}
