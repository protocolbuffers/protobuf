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
     * @dataProvider gettersAndSettersDataProvider
     */
    public function testGettersAndSetters(
        $class,
        $wrapperClass,
        $setter,
        $valueSetter,
        $getter,
        $valueGetter,
        $sequence
    ) {
        $oldSetterMsg = new $class();
        $newSetterMsg = new $class();
        foreach ($sequence as list($value, $expectedValue)) {
            // Manually wrap the value to pass to the old setter
            $wrappedValue = is_null($value) ? $value : new $wrapperClass(['value' => $value]);

            // Set values using new and old setters
            $oldSetterMsg->$setter($wrappedValue);
            $newSetterMsg->$valueSetter($value);

            // Get expected values old getter
            $expectedValue = $oldSetterMsg->$getter();

            // Check that old getter returns the same value after using the
            // new setter
            $actualValue = $newSetterMsg->$getter();
            $this->assertEquals($expectedValue, $actualValue);

            // Check that new getter returns the unwrapped value from
            // $expectedValue
            $actualValueNewGetter = $newSetterMsg->$valueGetter();
            if (is_null($expectedValue)) {
                $this->assertNull($actualValueNewGetter);
            } else {
                $this->assertEquals($expectedValue->getValue(), $actualValueNewGetter);
            }
        }
    }

    public function gettersAndSettersDataProvider()
    {
        return [
            [TestWrapperSetters::class, DoubleValue::class, "setDoubleValue", "setDoubleValueValue", "getDoubleValue", "getDoubleValueValue", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],
            [TestWrapperSetters::class, FloatValue::class, "setFloatValue", "setFloatValueValue", "getFloatValue", "getFloatValueValue", [
                [1.1, new FloatValue(["value" => 1.1])],
                [2.2, new FloatValue(["value" => 2.2])],
                [null, null],
                [0, new FloatValue()],
            ]],
            [TestWrapperSetters::class, Int64Value::class, "setInt64Value", "setInt64ValueValue", "getInt64Value", "getInt64ValueValue", [
                [123, new Int64Value(["value" => 123])],
                [-789, new Int64Value(["value" => -789])],
                [null, null],
                [0, new Int64Value()],
                [5.5, new Int64Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, UInt64Value::class, "setUInt64Value", "setUInt64ValueValue", "getUInt64Value", "getUInt64ValueValue", [
                [123, new UInt64Value(["value" => 123])],
                [789, new UInt64Value(["value" => 789])],
                [null, null],
                [0, new UInt64Value()],
                [5.5, new UInt64Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt64Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, Int32Value::class, "setInt32Value", "setInt32ValueValue", "getInt32Value", "getInt32ValueValue", [
                [123, new Int32Value(["value" => 123])],
                [-789, new Int32Value(["value" => -789])],
                [null, null],
                [0, new Int32Value()],
                [5.5, new Int32Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, UInt32Value::class, "setUInt32Value", "setUInt32ValueValue", "getUInt32Value", "getUInt32ValueValue", [
                [123, new UInt32Value(["value" => 123])],
                [789, new UInt32Value(["value" => 789])],
                [null, null],
                [0, new UInt32Value()],
                [5.5, new UInt32Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt32Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, BoolValue::class, "setBoolValue", "setBoolValueValue", "getBoolValue", "getBoolValueValue", [
                [true, new BoolValue(["value" => true])],
                [false, new BoolValue(["value" => false])],
                [null, null],
            ]],
            [TestWrapperSetters::class, StringValue::class, "setStringValue", "setStringValueValue", "getStringValue", "getStringValueValue", [
                ["asdf", new StringValue(["value" => "asdf"])],
                ["", new StringValue(["value" => ""])],
                [null, null],
                ["", new StringValue()],
                [5, new StringValue(["value" => "5"])], // Test conversion from number to string
                [5.5, new StringValue(["value" => "5.5"])], // Test conversion from number to string
                [-7, new StringValue(["value" => "-7"])], // Test conversion from number to string
                [-7.5, new StringValue(["value" => "-7.5"])], // Test conversion from number to string
            ]],
            [TestWrapperSetters::class, BytesValue::class, "setBytesValue", "setBytesValueValue", "getBytesValue", "getBytesValueValue", [
                ["asdf", new BytesValue(["value" => "asdf"])],
                ["", new BytesValue(["value" => ""])],
                [null, null],
                ["", new BytesValue()],
                [5, new BytesValue(["value" => "5"])], // Test conversion from number to bytes
                [5.5, new BytesValue(["value" => "5.5"])], // Test conversion from number to bytes
                [-7, new BytesValue(["value" => "-7"])], // Test conversion from number to bytes
                [-7.5, new BytesValue(["value" => "-7.5"])], // Test conversion from number to bytes
            ]],
            [TestWrapperSetters::class, DoubleValue::class, "setDoubleValueOneof", "setDoubleValueOneofValue", "getDoubleValueOneof", "getDoubleValueOneofValue", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],[TestWrapperSetters::class, StringValue::class, "setStringValueOneof", "setStringValueOneofValue", "getStringValueOneof", "getStringValueOneofValue", [
                ["asdf", new StringValue(["value" => "asdf"])],
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
            [TestWrapperSetters::class, "setDoubleValueValue", "abc"],
            [TestWrapperSetters::class, "setDoubleValueValue", []],
            [TestWrapperSetters::class, "setDoubleValueValue", new stdClass()],
            [TestWrapperSetters::class, "setDoubleValueValue", new DoubleValue()],

            [TestWrapperSetters::class, "setFloatValueValue", "abc"],
            [TestWrapperSetters::class, "setFloatValueValue", []],
            [TestWrapperSetters::class, "setFloatValueValue", new stdClass()],
            [TestWrapperSetters::class, "setFloatValueValue", new FloatValue()],

            [TestWrapperSetters::class, "setInt64ValueValue", "abc"],
            [TestWrapperSetters::class, "setInt64ValueValue", []],
            [TestWrapperSetters::class, "setInt64ValueValue", new stdClass()],
            [TestWrapperSetters::class, "setInt64ValueValue", new Int64Value()],

            [TestWrapperSetters::class, "setUInt64ValueValue", "abc"],
            [TestWrapperSetters::class, "setUInt64ValueValue", []],
            [TestWrapperSetters::class, "setUInt64ValueValue", new stdClass()],
            [TestWrapperSetters::class, "setUInt64ValueValue", new UInt64Value()],

            [TestWrapperSetters::class, "setInt32ValueValue", "abc"],
            [TestWrapperSetters::class, "setInt32ValueValue", []],
            [TestWrapperSetters::class, "setInt32ValueValue", new stdClass()],
            [TestWrapperSetters::class, "setInt32ValueValue", new Int32Value()],

            [TestWrapperSetters::class, "setUInt32ValueValue", "abc"],
            [TestWrapperSetters::class, "setUInt32ValueValue", []],
            [TestWrapperSetters::class, "setUInt32ValueValue", new stdClass()],
            [TestWrapperSetters::class, "setUInt32ValueValue", new UInt32Value()],

            [TestWrapperSetters::class, "setBoolValueValue", []],
            [TestWrapperSetters::class, "setBoolValueValue", new stdClass()],
            [TestWrapperSetters::class, "setBoolValueValue", new BoolValue()],

            [TestWrapperSetters::class, "setStringValueValue", []],
            [TestWrapperSetters::class, "setStringValueValue", new stdClass()],
            [TestWrapperSetters::class, "setStringValueValue", new StringValue()],

            [TestWrapperSetters::class, "setBytesValueValue", []],
            [TestWrapperSetters::class, "setBytesValueValue", new stdClass()],
            [TestWrapperSetters::class, "setBytesValueValue", new BytesValue()],
        ];
    }

    /**
     * @dataProvider constructorWithWrapperTypeDataProvider
     */
    public function testConstructorWithWrapperType($class, $wrapperClass, $wrapperField, $getter, $value)
    {
        $actualInstance = new $class([$wrapperField => $value]);
        $expectedInstance = new $class([$wrapperField => new $wrapperClass(['value' => $value])]);
        $this->assertEquals($expectedInstance->$getter()->getValue(), $actualInstance->$getter()->getValue());
    }

    public function constructorWithWrapperTypeDataProvider()
    {
        return [
            [TestWrapperSetters::class, DoubleValue::class, 'double_value', 'getDoubleValue', 1.1],
            [TestWrapperSetters::class, FloatValue::class, 'float_value', 'getFloatValue', 2.2],
            [TestWrapperSetters::class, Int64Value::class, 'int64_value', 'getInt64Value', 3],
            [TestWrapperSetters::class, UInt64Value::class, 'uint64_value', 'getUInt64Value', 4],
            [TestWrapperSetters::class, Int32Value::class, 'int32_value', 'getInt32Value', 5],
            [TestWrapperSetters::class, UInt32Value::class, 'uint32_value', 'getUInt32Value', 6],
            [TestWrapperSetters::class, BoolValue::class, 'bool_value', 'getBoolValue', true],
            [TestWrapperSetters::class, StringValue::class, 'string_value', 'getStringValue', "eight"],
            [TestWrapperSetters::class, BytesValue::class, 'bytes_value', 'getBytesValue', "nine"],
        ];
    }
}
