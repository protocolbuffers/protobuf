<?php

require_once('test_base.php');
require_once('test_util.php');

use Foo\TestWrapperSetters;
use Google\Protobuf\BoolValue;
use Google\Protobuf\BytesValue;
use Google\Protobuf\DoubleValue;
use Google\Protobuf\FloatValue;
use Google\Protobuf\Int32Value;
use Google\Protobuf\Int64Value;
use Google\Protobuf\StringValue;
use Google\Protobuf\UInt32Value;
use Google\Protobuf\UInt64Value;

class WrapperTypeSettersTest extends TestBase
{
    /**
     * @dataProvider settersDataProvider
     */
    public function testSetters($class, $sequence)
    {
        $msg = new $class();
        foreach ($sequence as list($setter, $getter, $value, $expectedValue)) {
            $actualValue = $msg->$setter($value)->$getter();
            $this->assertEquals($expectedValue, $actualValue);
        }
    }

    public function settersDataProvider()
    {
        return [
            [TestWrapperSetters::class, [
                ["setDoubleValue", "getDoubleValue", 1.1, new DoubleValue(["value" => 1.1])],
                ["setDoubleValue", "getDoubleValue", new DoubleValue(["value" => 3.3]), new DoubleValue(["value" => 3.3])],
                ["setDoubleValue", "getDoubleValue", 2.2, new DoubleValue(["value" => 2.2])],
                ["setDoubleValue", "getDoubleValue", null, null],
                ["setDoubleValue", "getDoubleValue", 0, new DoubleValue()],
            ]],
            [TestWrapperSetters::class, [
                ["setFloatValue", "getFloatValue", 1.1, new FloatValue(["value" => 1.1])],
                ["setFloatValue", "getFloatValue", new FloatValue(["value" => 3.3]), new FloatValue(["value" => 3.3])],
                ["setFloatValue", "getFloatValue", 2.2, new FloatValue(["value" => 2.2])],
                ["setFloatValue", "getFloatValue", null, null],
                ["setFloatValue", "getFloatValue", 0, new FloatValue()],
            ]],
            [TestWrapperSetters::class, [
                ["setInt64Value", "getInt64Value", 123, new Int64Value(["value" => 123])],
                ["setInt64Value", "getInt64Value", new Int64Value(["value" => 23456]), new Int64Value(["value" => 23456])],
                ["setInt64Value", "getInt64Value", -789, new Int64Value(["value" => -789])],
                ["setInt64Value", "getInt64Value", null, null],
                ["setInt64Value", "getInt64Value", 0, new Int64Value()],
            ]],
            [TestWrapperSetters::class, [
                ["setUInt64Value", "getUInt64Value", 123, new UInt64Value(["value" => 123])],
                ["setUInt64Value", "getUInt64Value", new UInt64Value(["value" => 23456]), new UInt64Value(["value" => 23456])],
                ["setUInt64Value", "getUInt64Value", 789, new UInt64Value(["value" => 789])],
                ["setUInt64Value", "getUInt64Value", null, null],
                ["setUInt64Value", "getUInt64Value", 0, new UInt64Value()],
            ]],
            [TestWrapperSetters::class, [
                ["setInt32Value", "getInt32Value", 123, new Int32Value(["value" => 123])],
                ["setInt32Value", "getInt32Value", new Int32Value(["value" => 23456]), new Int32Value(["value" => 23456])],
                ["setInt32Value", "getInt32Value", -789, new Int32Value(["value" => -789])],
                ["setInt32Value", "getInt32Value", null, null],
                ["setInt32Value", "getInt32Value", 0, new Int32Value()],
            ]],
            [TestWrapperSetters::class, [
                ["setUInt32Value", "getUInt32Value", 123, new UInt32Value(["value" => 123])],
                ["setUInt32Value", "getUInt32Value", new UInt32Value(["value" => 23456]), new UInt32Value(["value" => 23456])],
                ["setUInt32Value", "getUInt32Value", 789, new UInt32Value(["value" => 789])],
                ["setUInt32Value", "getUInt32Value", null, null],
                ["setUInt32Value", "getUInt32Value", 0, new UInt32Value()],
            ]],
            [TestWrapperSetters::class, [
                ["setBoolValue", "getBoolValue", true, new BoolValue(["value" => true])],
                ["setBoolValue", "getBoolValue", new BoolValue(["value" => true]), new BoolValue(["value" => true])],
                ["setBoolValue", "getBoolValue", false, new BoolValue(["value" => false])],
                ["setBoolValue", "getBoolValue", null, null],
            ]],
            [TestWrapperSetters::class, [
                ["setStringValue", "getStringValue", "asdf", new StringValue(["value" => "asdf"])],
                ["setStringValue", "getStringValue", new StringValue(["value" => "qwerty"]), new StringValue(["value" => "qwerty"])],
                ["setStringValue", "getStringValue", "", new StringValue(["value" => ""])],
                ["setStringValue", "getStringValue", null, null],
                ["setStringValue", "getStringValue", "", new StringValue()],
            ]],
            [TestWrapperSetters::class, [
                ["setBytesValue", "getBytesValue", "asdf", new BytesValue(["value" => "asdf"])],
                ["setBytesValue", "getBytesValue", new BytesValue(["value" => "qwerty"]), new BytesValue(["value" => "qwerty"])],
                ["setBytesValue", "getBytesValue", "", new BytesValue(["value" => ""])],
                ["setBytesValue", "getBytesValue", null, null],
                ["setBytesValue", "getBytesValue", "", new BytesValue()],
            ]],
            [TestWrapperSetters::class, [
                ["setDoubleValueList", "getDoubleValueList", [1.1], [new DoubleValue(["value" => 1.1])]],
//                ["setDoubleValueList", "getDoubleValueList", new DoubleValue(["value" => 3.3]), new DoubleValue(["value" => 3.3])],
//                ["setDoubleValueList", "getDoubleValueList", 2.2, new DoubleValue(["value" => 2.2])],
//                ["setDoubleValueList", "getDoubleValueList", null, null],
//                ["setDoubleValueList", "getDoubleValueList", 0, new DoubleValue()],
            ]],
        ];
    }

    /**
     * @dataProvider invalidSettersDataProvider
     * @expectedException \Exception
     */
    public function testInvalidSetters($class, $setter, $value)
    {
        (new $class())->$setter($value);
    }

    public function invalidSettersDataProvider()
    {
        return [
            [TestWrapperSetters::class, "setDoubleValue", "abc"],
            [TestWrapperSetters::class, "setDoubleValue", []],
            [TestWrapperSetters::class, "setDoubleValue", new stdClass()],

            [TestWrapperSetters::class, "setFloatValue", "abc"],
            [TestWrapperSetters::class, "setFloatValue", []],
            [TestWrapperSetters::class, "setFloatValue", new stdClass()],

            [TestWrapperSetters::class, "setInt64Value", "abc"],
//            [TestWrapperSetters::class, "setInt64Value", 5.5],
            [TestWrapperSetters::class, "setInt64Value", []],
            [TestWrapperSetters::class, "setInt64Value", new stdClass()],

            [TestWrapperSetters::class, "setUInt64Value", "abc"],
//            [TestWrapperSetters::class, "setUInt64Value", 5.5],
//            [TestWrapperSetters::class, "setUInt64Value", -7],
            [TestWrapperSetters::class, "setUInt64Value", []],
            [TestWrapperSetters::class, "setUInt64Value", new stdClass()],

            [TestWrapperSetters::class, "setInt32Value", "abc"],
//            [TestWrapperSetters::class, "setInt32Value", 5.5],
            [TestWrapperSetters::class, "setInt32Value", []],
            [TestWrapperSetters::class, "setInt32Value", new stdClass()],

            [TestWrapperSetters::class, "setUInt32Value", "abc"],
//            [TestWrapperSetters::class, "setUInt32Value", 5.5],
//            [TestWrapperSetters::class, "setUInt32Value", -7],
            [TestWrapperSetters::class, "setUInt32Value", []],
            [TestWrapperSetters::class, "setUInt32Value", new stdClass()],

            [TestWrapperSetters::class, "setBoolValue", []],
            [TestWrapperSetters::class, "setBoolValue", new stdClass()],

//            [TestWrapperSetters::class, "setStringValue", 5],
//            [TestWrapperSetters::class, "setStringValue", 5.5],
            [TestWrapperSetters::class, "setStringValue", []],
            [TestWrapperSetters::class, "setStringValue", new stdClass()],

//            [TestWrapperSetters::class, "setBytesValue", 5],
//            [TestWrapperSetters::class, "setBytesValue", 5.5],
            [TestWrapperSetters::class, "setBytesValue", []],
            [TestWrapperSetters::class, "setBytesValue", new stdClass()],
        ];
    }
}
