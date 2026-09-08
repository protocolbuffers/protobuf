<?php

namespace Google\Protobuf\Internal;

class EnumDescriptor
{
    use HasPublicDescriptorTrait;

    private $klass;
    private $legacy_klass;
    private $full_name;
    private $value;
    private $name_to_value;
    private $custom_name_to_value = [];
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

    /**
     * @param EnumValueDescriptor $value
     */
    public function addValue(EnumValueDescriptor $value)
    {
        $this->value[$value->getNumber()] = $value;
        $this->name_to_value[$value->getName()] = $value;
        $this->value_descriptor[] = $value;

        $custom_name = $value->getCustomJsonName();
        if ($custom_name !== null) {
            $this->custom_name_to_value[$custom_name] = $value;
        }
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

    /**
     * Looks up an enum value descriptor by its JSON name.
     *
     * @param string $name
     * @return EnumValueDescriptor|null
     */
    public function getValueByJsonName($name)
    {
        if (isset($this->custom_name_to_value[$name])) {
            return $this->custom_name_to_value[$name];
        }
        return $this->getValueByName($name);
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

    public static function buildFromProto($proto, $file_proto, $containing, $custom_json_names = [])
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
            $val_fqn = $fullname . '.' . $value->getName();
            $custom_json_name = $custom_json_names[$val_fqn] ?? null;
            $desc->addValue(new EnumValueDescriptor(
                $value->getName(), $value->getNumber(), $custom_json_name));
        }

        return $desc;
    }
}
