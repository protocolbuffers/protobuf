<?php

namespace Google\Protobuf\Internal;

use Google\Protobuf\EnumValueDescriptor;

class EnumDescriptor
{
    use HasPublicDescriptorTrait;

    private $klass;
    private $legacy_klass;
    private $full_name;
    private $value;
    private $name_to_value;
    private $value_descriptor = [];

    public function __construct()
    {
        $this->public_desc = new \Google\Protobuf\EnumDescriptor($this);
    }

    public function setFullName($full_name)
    {
        $this->full_name = $full_name;
    }

    public function getFullName()
    {
        return $this->full_name;
    }

    public function addValue($number, $value)
    {
        $this->value[$number] = $value;
        $this->name_to_value[$value->getName()] = $value;
        $this->value_descriptor[] = new EnumValueDescriptor($value->getName(), $number);
    }

    public function getValueByNumber($number)
    {
        if (isset($this->value[$number])) {
            return $this->value[$number];
        }
        return null;
    }

    public function getValueByName($name)
    {
        if (isset($this->name_to_value[$name])) {
            return $this->name_to_value[$name];
        }
        return null;
    }

    public function getValueDescriptorByIndex($index)
    {
        if (isset($this->value_descriptor[$index])) {
            return $this->value_descriptor[$index];
        }
        return null;
    }

    public function getValueCount()
    {
        return count($this->value);
    }

    public function setClass($klass)
    {
        $this->klass = $klass;
    }

    public function getClass()
    {
        return $this->klass;
    }

    public function setLegacyClass($klass)
    {
        $this->legacy_klass = $klass;
    }

    public function getLegacyClass()
    {
        return $this->legacy_klass;
    }

    public static function buildFromProto($proto, $file_proto, $containing)
    {
        $desc = new EnumDescriptor();

        $enum_name_without_package  = "";
        $classname = "";
        $legacy_classname = "";
        $fullname = "";
        GPBUtil::getFullClassName(
            $proto,
            $containing,
            $file_proto,
            $enum_name_without_package,
            $classname,
            $legacy_classname,
            $fullname,
            $unused_previous_classname);
        $desc->setFullName($fullname);
        $desc->setClass($classname);
        $desc->setLegacyClass($legacy_classname);
        $values = $proto->getValue();
        foreach ($values as $value) {
            $desc->addValue($value->getNumber(), $value);
        }

        return $desc;
    }
}
