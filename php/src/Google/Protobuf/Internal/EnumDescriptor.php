<?php

namespace Google\Protobuf\Internal;

class EnumDescriptor
{

    private $klass;
    private $full_name;
    private $value;

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

        return $desc;
    }
}
