<?php

namespace Google\Protobuf\Internal;

class EnumDescriptor
{

    private $klass;
    private $full_name;
    private $value;
    private $name_to_value;

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
    }

    public function getValueByNumber($number)
    {
        return $this->value[$number];
    }

    public function getValueByName($name)
    {
        return $this->name_to_value[$name];
    }

    public function setClass($klass)
    {
        $this->klass = $klass;
    }

    public function getClass()
    {
        return $this->klass;
    }

    public static function buildFromProto($proto, $file_proto, $containing)
    {
        $desc = new EnumDescriptor();

        $enum_name_without_package  = "";
        $classname = "";
        $fullname = "";
        GPBUtil::getFullClassName(
            $proto,
            $containing,
            $file_proto,
            $enum_name_without_package,
            $classname,
            $fullname);
        $desc->setFullName($fullname);
        $desc->setClass($classname);
        $values = $proto->getValue();
        foreach ($values as $value) {
            $desc->addValue($value->getNumber(), $value);
        }

        return $desc;
    }
}
