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
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\StringValue;
use Google\Protobuf\UInt32Value;
use Google\Protobuf\UInt64Value;

class WrapperTypeSettersTest extends TestBase
{
    /**
     * @dataProvider gettersAndSettersDataProvider
     */
    public function testGettersAndSetters($class, $setter, $getter, $valueGetter, $sequence)
    {
        $msg = new $class();
        foreach ($sequence as list($value, $expectedValue)) {
            $actualValue = $msg->$setter($value)->$getter();
            $actualValueGetterValue = $msg->$valueGetter();
            $this->assertEquals($expectedValue, $actualValue);
            if (is_null($expectedValue)) {
                $this->assertNull($actualValueGetterValue);
            } else {
                $this->assertEquals($expectedValue->getValue(), $actualValueGetterValue);
            }
        }
    }

    public function gettersAndSettersDataProvider()
    {
        return [
            [TestWrapperSetters::class, "setDoubleValue", "getDoubleValue", "getDoubleValueValue", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [new DoubleValue(["value" => 3.3]), new DoubleValue(["value" => 3.3])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],
            [TestWrapperSetters::class, "setFloatValue", "getFloatValue", "getFloatValueValue", [
                [1.1, new FloatValue(["value" => 1.1])],
                [new FloatValue(["value" => 3.3]), new FloatValue(["value" => 3.3])],
                [2.2, new FloatValue(["value" => 2.2])],
                [null, null],
                [0, new FloatValue()],
            ]],
            [TestWrapperSetters::class, "setInt64Value", "getInt64Value", "getInt64ValueValue", [
                [123, new Int64Value(["value" => 123])],
                [new Int64Value(["value" => 23456]), new Int64Value(["value" => 23456])],
                [-789, new Int64Value(["value" => -789])],
                [null, null],
                [0, new Int64Value()],
                [5.5, new Int64Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, "setUInt64Value", "getUInt64Value", "getUInt64ValueValue", [
                [123, new UInt64Value(["value" => 123])],
                [new UInt64Value(["value" => 23456]), new UInt64Value(["value" => 23456])],
                [789, new UInt64Value(["value" => 789])],
                [null, null],
                [0, new UInt64Value()],
                [5.5, new UInt64Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt64Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, "setInt32Value", "getInt32Value", "getInt32ValueValue", [
                [123, new Int32Value(["value" => 123])],
                [new Int32Value(["value" => 23456]), new Int32Value(["value" => 23456])],
                [-789, new Int32Value(["value" => -789])],
                [null, null],
                [0, new Int32Value()],
                [5.5, new Int32Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, "setUInt32Value", "getUInt32Value", "getUInt32ValueValue", [
                [123, new UInt32Value(["value" => 123])],
                [new UInt32Value(["value" => 23456]), new UInt32Value(["value" => 23456])],
                [789, new UInt32Value(["value" => 789])],
                [null, null],
                [0, new UInt32Value()],
                [5.5, new UInt32Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt32Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, "setBoolValue", "getBoolValue", "getBoolValueValue", [
                [true, new BoolValue(["value" => true])],
                [new BoolValue(["value" => true]), new BoolValue(["value" => true])],
                [false, new BoolValue(["value" => false])],
                [null, null],
            ]],
            [TestWrapperSetters::class, "setStringValue", "getStringValue", "getStringValueValue", [
                ["asdf", new StringValue(["value" => "asdf"])],
                [new StringValue(["value" => "qwerty"]), new StringValue(["value" => "qwerty"])],
                ["", new StringValue(["value" => ""])],
                [null, null],
                ["", new StringValue()],
                [5, new StringValue(["value" => "5"])], // Test conversion from number to string
                [5.5, new StringValue(["value" => "5.5"])], // Test conversion from number to string
                [-7, new StringValue(["value" => "-7"])], // Test conversion from number to string
                [-7.5, new StringValue(["value" => "-7.5"])], // Test conversion from number to string
            ]],
            [TestWrapperSetters::class, "setBytesValue", "getBytesValue", "getBytesValueValue", [
                ["asdf", new BytesValue(["value" => "asdf"])],
                [new BytesValue(["value" => "qwerty"]), new BytesValue(["value" => "qwerty"])],
                ["", new BytesValue(["value" => ""])],
                [null, null],
                ["", new BytesValue()],
                [5, new BytesValue(["value" => "5"])], // Test conversion from number to bytes
                [5.5, new BytesValue(["value" => "5.5"])], // Test conversion from number to bytes
                [-7, new BytesValue(["value" => "-7"])], // Test conversion from number to bytes
                [-7.5, new BytesValue(["value" => "-7.5"])], // Test conversion from number to bytes
            ]],
            [TestWrapperSetters::class, "setDoubleValueOneof", "getDoubleValueOneof", "getDoubleValueOneofValue", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [new DoubleValue(["value" => 3.3]), new DoubleValue(["value" => 3.3])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],[TestWrapperSetters::class, "setStringValueOneof", "getStringValueOneof", "getStringValueOneofValue", [
                ["asdf", new StringValue(["value" => "asdf"])],
                [new StringValue(["value" => "qwerty"]), new StringValue(["value" => "qwerty"])],
                ["", new StringValue(["value" => ""])],
                [null, null],
                ["", new StringValue()],
                [5, new StringValue(["value" => "5"])], // Test conversion from number to string
                [5.5, new StringValue(["value" => "5.5"])], // Test conversion from number to string
                [-7, new StringValue(["value" => "-7"])], // Test conversion from number to string
                [-7.5, new StringValue(["value" => "-7.5"])], // Test conversion from number to string
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
            [TestWrapperSetters::class, "setInt64Value", []],
            [TestWrapperSetters::class, "setInt64Value", new stdClass()],

            [TestWrapperSetters::class, "setUInt64Value", "abc"],
            [TestWrapperSetters::class, "setUInt64Value", []],
            [TestWrapperSetters::class, "setUInt64Value", new stdClass()],

            [TestWrapperSetters::class, "setInt32Value", "abc"],
            [TestWrapperSetters::class, "setInt32Value", []],
            [TestWrapperSetters::class, "setInt32Value", new stdClass()],

            [TestWrapperSetters::class, "setUInt32Value", "abc"],
            [TestWrapperSetters::class, "setUInt32Value", []],
            [TestWrapperSetters::class, "setUInt32Value", new stdClass()],

            [TestWrapperSetters::class, "setBoolValue", []],
            [TestWrapperSetters::class, "setBoolValue", new stdClass()],

            [TestWrapperSetters::class, "setStringValue", []],
            [TestWrapperSetters::class, "setStringValue", new stdClass()],

            [TestWrapperSetters::class, "setBytesValue", []],
            [TestWrapperSetters::class, "setBytesValue", new stdClass()],
        ];
    }
}
