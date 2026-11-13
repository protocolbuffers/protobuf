<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\Edition;
use Google\Protobuf\Internal\FeatureSet\FieldPresence;
use Google\Protobuf\Internal\FeatureSet\RepeatedFieldEncoding;

class FieldDescriptor
{
    use HasPublicDescriptorTrait;

    private $name;
    private $json_name;
    private $setter;
    private $getter;
    private $number;
    private $label;
    private $type;
    private $message_type;
    private $enum_type;
    private $packed;
    private $oneof_index = -1;
    private $proto3_optional;
    private $has_presence = false;

    /** @var OneofDescriptor $containing_oneof */
    private $containing_oneof;

    public function __construct()
    {
        $this->public_desc = new \Google\Protobuf\FieldDescriptor($this);
    }

    public function setOneofIndex($index)
    {
        $this->oneof_index = $index;
    }

    public function getOneofIndex()
    {
        return $this->oneof_index;
    }

    public function setName($name)
    {
        $this->name = $name;
    }

    public function getName()
    {
        return $this->name;
    }

    public function setJsonName($json_name)
    {
        $this->json_name = $json_name;
    }

    public function getJsonName()
    {
        return $this->json_name;
    }

    public function setSetter($setter)
    {
        $this->setter = $setter;
    }

    public function getSetter()
    {
        return $this->setter;
    }

    public function setGetter($getter)
    {
        $this->getter = $getter;
    }

    public function getGetter()
    {
        return $this->getter;
    }

    public function setNumber($number)
    {
        $this->number = $number;
    }

    public function getNumber()
    {
        return $this->number;
    }

    public function setLabel($label)
    {
        $this->label = $label;
    }

    public function isRequired()
    {
        return $this->label === GPBLabel::REQUIRED;
    }

    public function isRepeated()
    {
        return $this->label === GPBLabel::REPEATED;
    }

    public function setType($type)
    {
        $this->type = $type;
    }

    public function getType()
    {
        return $this->type;
    }

    public function setMessageType($message_type)
    {
        $this->message_type = $message_type;
    }

    public function getMessageType()
    {
        return $this->message_type;
    }

    public function setEnumType($enum_type)
    {
        $this->enum_type = $enum_type;
    }

    public function getEnumType()
    {
        return $this->enum_type;
    }

    public function setPacked($packed)
    {
        $this->packed = $packed;
    }

    public function getPacked()
    {
        return $this->packed;
    }

    public function getProto3Optional()
    {
        return $this->proto3_optional;
    }

    public function setProto3Optional($proto3_optional)
    {
        $this->proto3_optional = $proto3_optional;
    }

    public function setHasPresence(bool $has_presence)
    {
        $this->has_presence = $has_presence;
    }

    public function hasPresence()
    {
        if ($this->isRepeated()) {
            return false;
        }
        return $this->has_presence ||
               $this->getType() == GPBType::MESSAGE ||
               $this->getType() == GPBType::GROUP ||
               !is_null($this->containing_oneof);
    }

    public function getContainingOneof()
    {
        return $this->containing_oneof;
    }

    public function setContainingOneof($containing_oneof)
    {
        $this->containing_oneof = $containing_oneof;
    }

    public function getRealContainingOneof()
    {
        return !is_null($this->containing_oneof) && !$this->containing_oneof->isSynthetic()
            ? $this->containing_oneof : null;
    }

    public function isPackable()
    {
        return $this->isRepeated() && self::isTypePackable($this->type);
    }

    public function isMap()
    {
        return $this->getType() == GPBType::MESSAGE &&
               !is_null($this->getMessageType()->getOptions()) &&
               $this->getMessageType()->getOptions()->getMapEntry();
    }

    public function isTimestamp()
    {
        return $this->getType() == GPBType::MESSAGE &&
            $this->getMessageType()->getClass() === "Google\Protobuf\Timestamp";
    }

    public function isWrapperType()
    {
        if ($this->getType() == GPBType::MESSAGE) {
            $class = $this->getMessageType()->getClass();
            return in_array($class, [
                "Google\Protobuf\DoubleValue",
                "Google\Protobuf\FloatValue",
                "Google\Protobuf\Int64Value",
                "Google\Protobuf\UInt64Value",
                "Google\Protobuf\Int32Value",
                "Google\Protobuf\UInt32Value",
                "Google\Protobuf\BoolValue",
                "Google\Protobuf\StringValue",
                "Google\Protobuf\BytesValue",
            ]);
        }
        return false;
    }

    private static function isTypePackable($field_type)
    {
        return ($field_type !== GPBType::STRING  &&
            $field_type !== GPBType::GROUP   &&
            $field_type !== GPBType::MESSAGE &&
            $field_type !== GPBType::BYTES);
    }

    /**
     * @param FieldDescriptorProto $proto
     * @return FieldDescriptor
     */
    public static function getFieldDescriptor($proto)
    {
        return self::buildWithFeatures(
            $proto,
            FileDescriptor::defaultFeatures(Edition::EDITION_PROTO3),
            Edition::EDITION_PROTO3);
    }

    /**
     * @param FieldDescriptorProto $proto
     * @param FeatureSet $parent_features
     * @param int $edition
     * @return FieldDescriptor
     */
    public static function buildWithFeatures(
        $proto,
        FeatureSet $parent_features,
        int $edition
    ) {
        $type_name = null;
        $type = $proto->getType();
        switch ($type) {
            case GPBType::MESSAGE:
            case GPBType::GROUP:
            case GPBType::ENUM:
                $type_name = $proto->getTypeName();
                break;
            default:
                break;
        }

        $oneof_index = $proto->hasOneofIndex() ? $proto->getOneofIndex() : -1;
        $options = $proto->getOptions();
        $features = FileDescriptor::resolveFeatures(
            $parent_features,
            $options !== null ? $options->getFeatures() : null
        );

        $label = $proto->getLabel();
        if ($edition === Edition::EDITION_PROTO2 ||
            $edition === Edition::EDITION_PROTO3) {
            if ($label === GPBLabel::REQUIRED) {
                $features = clone $features;
                $features->setFieldPresence(FieldPresence::LEGACY_REQUIRED);
            }
            if ($options !== null && $options->hasPacked()) {
                $features = clone $features;
                $features->setRepeatedFieldEncoding(
                    $options->getPacked()
                        ? RepeatedFieldEncoding::PACKED
                        : RepeatedFieldEncoding::EXPANDED
                );
            }
        } elseif ($label === GPBLabel::OPTIONAL &&
                  $features->getFieldPresence() === FieldPresence::LEGACY_REQUIRED) {
            $label = GPBLabel::REQUIRED;
        }

        if ($label === GPBLabel::REPEATED && self::isTypePackable($type)) {
            $packed = ($features->getRepeatedFieldEncoding() === RepeatedFieldEncoding::PACKED);
        } else {
            $packed = false;
        }

        $field = new FieldDescriptor();
        $field->setName($proto->getName());

        if ($proto->hasJsonName()) {
            $json_name = $proto->getJsonName();
        } else {
            $proto_name = $proto->getName();
            $json_name = implode('', array_map('ucwords', explode('_', $proto_name)));
            if ($proto_name[0] !== "_" && !ctype_upper($proto_name[0])) {
                $json_name = lcfirst($json_name);
            }
        }
        $field->setJsonName($json_name);

        $camel_name = implode('', array_map('ucwords', explode('_', $proto->getName())));
        $field->setGetter('get' . $camel_name);
        $field->setSetter('set' . $camel_name);
        $field->setType($proto->getType());
        $field->setNumber($proto->getNumber());
        $field->setLabel($label);
        $field->setPacked($packed);
        $field->setOneofIndex($oneof_index);
        $field->setProto3Optional($proto->getProto3Optional());
        $field->setHasPresence(
            $label !== GPBLabel::REPEATED &&
            $features->getFieldPresence() !== FieldPresence::IMPLICIT
        );

        // At this time, the message/enum type may have not been added to pool.
        // So we use the type name as place holder and will replace it with the
        // actual descriptor in cross building.
        switch ($type) {
            case GPBType::MESSAGE:
                $field->setMessageType($type_name);
                break;
            case GPBType::ENUM:
                $field->setEnumType($type_name);
                break;
            default:
                break;
        }

        return $field;
    }

    public static function buildFromProto($proto)
    {
        return FieldDescriptor::getFieldDescriptor($proto);
    }
}
