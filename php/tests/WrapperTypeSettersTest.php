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
            $wrappedValue = is_null($value) ? $value : @new $wrapperClass(['value' => $value]);

            // Set values using new and old setters
            $oldSetterMsg->$setter($wrappedValue);
            @$newSetterMsg->$valueSetter($value);

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

    public static function gettersAndSettersDataProvider()
    {
        return [
            [TestWrapperSetters::class, DoubleValue::class, "setDoubleValue", "setDoubleValueUnwrapped", "getDoubleValue", "getDoubleValueUnwrapped", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],
            [TestWrapperSetters::class, FloatValue::class, "setFloatValue", "setFloatValueUnwrapped", "getFloatValue", "getFloatValueUnwrapped", [
                [1.1, new FloatValue(["value" => 1.1])],
                [2.2, new FloatValue(["value" => 2.2])],
                [null, null],
                [0, new FloatValue()],
            ]],
            [TestWrapperSetters::class, Int64Value::class, "setInt64Value", "setInt64ValueUnwrapped", "getInt64Value", "getInt64ValueUnwrapped", [
                [123, new Int64Value(["value" => 123])],
                [-789, new Int64Value(["value" => -789])],
                [null, null],
                [0, new Int64Value()],
                [5.5, new Int64Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, UInt64Value::class, "setUInt64Value", "setUInt64ValueUnwrapped", "getUInt64Value", "getUInt64ValueUnwrapped", [
                [123, new UInt64Value(["value" => 123])],
                [789, new UInt64Value(["value" => 789])],
                [null, null],
                [0, new UInt64Value()],
                [5.5, new UInt64Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt64Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, Int32Value::class, "setInt32Value", "setInt32ValueUnwrapped", "getInt32Value", "getInt32ValueUnwrapped", [
                [123, new Int32Value(["value" => 123])],
                [-789, new Int32Value(["value" => -789])],
                [null, null],
                [0, new Int32Value()],
                [5.5, new Int32Value(["value" => 5])], // Test conversion from float to int
            ]],
            [TestWrapperSetters::class, UInt32Value::class, "setUInt32Value", "setUInt32ValueUnwrapped", "getUInt32Value", "getUInt32ValueUnwrapped", [
                [123, new UInt32Value(["value" => 123])],
                [789, new UInt32Value(["value" => 789])],
                [null, null],
                [0, new UInt32Value()],
                [5.5, new UInt32Value(["value" => 5])], // Test conversion from float to int
                [-7, new UInt32Value(["value" => -7])], // Test conversion from -ve to +ve
            ]],
            [TestWrapperSetters::class, BoolValue::class, "setBoolValue", "setBoolValueUnwrapped", "getBoolValue", "getBoolValueUnwrapped", [
                [true, new BoolValue(["value" => true])],
                [false, new BoolValue(["value" => false])],
                [null, null],
            ]],
            [TestWrapperSetters::class, StringValue::class, "setStringValue", "setStringValueUnwrapped", "getStringValue", "getStringValueUnwrapped", [
                ["asdf", new StringValue(["value" => "asdf"])],
                ["", new StringValue(["value" => ""])],
                [null, null],
                ["", new StringValue()],
                [5, new StringValue(["value" => "5"])], // Test conversion from number to string
                [5.5, new StringValue(["value" => "5.5"])], // Test conversion from number to string
                [-7, new StringValue(["value" => "-7"])], // Test conversion from number to string
                [-7.5, new StringValue(["value" => "-7.5"])], // Test conversion from number to string
            ]],
            [TestWrapperSetters::class, BytesValue::class, "setBytesValue", "setBytesValueUnwrapped", "getBytesValue", "getBytesValueUnwrapped", [
                ["asdf", new BytesValue(["value" => "asdf"])],
                ["", new BytesValue(["value" => ""])],
                [null, null],
                ["", new BytesValue()],
                [5, new BytesValue(["value" => "5"])], // Test conversion from number to bytes
                [5.5, new BytesValue(["value" => "5.5"])], // Test conversion from number to bytes
                [-7, new BytesValue(["value" => "-7"])], // Test conversion from number to bytes
                [-7.5, new BytesValue(["value" => "-7.5"])], // Test conversion from number to bytes
            ]],
            [TestWrapperSetters::class, DoubleValue::class, "setDoubleValueOneof", "setDoubleValueOneofUnwrapped", "getDoubleValueOneof", "getDoubleValueOneofUnwrapped", [
                [1.1, new DoubleValue(["value" => 1.1])],
                [2.2, new DoubleValue(["value" => 2.2])],
                [null, null],
                [0, new DoubleValue()],
            ]],
            [TestWrapperSetters::class, StringValue::class, "setStringValueOneof", "setStringValueOneofUnwrapped", "getStringValueOneof", "getStringValueOneofUnwrapped", [
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

    public function testUint64ValueUnwrappedInvalidString()
    {
        $this->expectException(Exception::class);
        if (extension_loaded('protobuf')) {
            $this->expectExceptionMessage('Cannot convert ');
        } else {
            $this->expectExceptionMessage('Expect ');
        }
        (new TestWrapperSetters())->setUInt64ValueUnwrapped('abc');
    }

    /**
     * @dataProvider invalidSettersDataProvider
     */
    public function testInvalidSetters($setter, $value)
    {
        if (extension_loaded('protobuf')) {
            $this->expectException(Exception::class);
            $this->expectExceptionMessage('Cannot convert ');
        } else {
            $this->expectException(TypeError::class);
        }

        (new TestWrapperSetters())->$setter($value);
    }

    public static function invalidSettersDataProvider()
    {
        return [
            ["setDoubleValueUnwrapped", "abc"],
            ["setDoubleValueUnwrapped", []],
            ["setDoubleValueUnwrapped", new stdClass()],
            ["setDoubleValueUnwrapped", new DoubleValue()],

            ["setFloatValueUnwrapped", "abc"],
            ["setFloatValueUnwrapped", []],
            ["setFloatValueUnwrapped", new stdClass()],
            ["setFloatValueUnwrapped", new FloatValue()],

            // ["setUInt64ValueUnwrapped", "abc"], // Tested in testUint64ValueUnwrappedInvalidString
            ["setUInt64ValueUnwrapped", []],
            ["setUInt64ValueUnwrapped", new stdClass()],
            ["setUInt64ValueUnwrapped", new UInt64Value()],

            ["setInt32ValueUnwrapped", "abc"],
            ["setInt32ValueUnwrapped", []],
            ["setInt32ValueUnwrapped", new stdClass()],
            ["setInt32ValueUnwrapped", new Int32Value()],

            ["setUInt32ValueUnwrapped", "abc"],
            ["setUInt32ValueUnwrapped", []],
            ["setUInt32ValueUnwrapped", new stdClass()],
            ["setUInt32ValueUnwrapped", new UInt32Value()],

            ["setBoolValueUnwrapped", []],
            ["setBoolValueUnwrapped", new stdClass()],
            ["setBoolValueUnwrapped", new BoolValue()],

            ["setStringValueUnwrapped", []],
            ["setStringValueUnwrapped", new stdClass()],
            ["setStringValueUnwrapped", new StringValue()],

            ["setBytesValueUnwrapped", []],
            ["setBytesValueUnwrapped", new stdClass()],
            ["setBytesValueUnwrapped", new BytesValue()],
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

    public static function constructorWithWrapperTypeDataProvider()
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

    /**
     * @dataProvider constructorWithRepeatedWrapperTypeDataProvider
     */
    public function testConstructorWithRepeatedWrapperType($wrapperField, $getter, $value)
    {
        $actualInstance = new TestWrapperSetters([$wrapperField => $value]);
        foreach ($actualInstance->$getter() as $key => $actualWrapperValue) {
            $actualInnerValue = $actualWrapperValue->getValue();
            $expectedElement = $value[$key];
            if (is_object($expectedElement) && is_a($expectedElement, '\Google\Protobuf\StringValue')) {
                $expectedInnerValue = $expectedElement->getValue();
            } else {
                $expectedInnerValue = $expectedElement;
            }
            $this->assertEquals($expectedInnerValue, $actualInnerValue);
        }

        $this->assertTrue(true);
    }

    public static function constructorWithRepeatedWrapperTypeDataProvider()
    {
        $sv7 = new StringValue(['value' => 'seven']);
        $sv8 = new StringValue(['value' => 'eight']);

        $testWrapperSetters = new TestWrapperSetters();
        $testWrapperSetters->setRepeatedStringValue([$sv7, $sv8]);
        $repeatedField = $testWrapperSetters->getRepeatedStringValue();

        return [
            ['repeated_string_value', 'getRepeatedStringValue', []],
            ['repeated_string_value', 'getRepeatedStringValue', [$sv7]],
            ['repeated_string_value', 'getRepeatedStringValue', [$sv7, $sv8]],
            ['repeated_string_value', 'getRepeatedStringValue', ['seven']],
            ['repeated_string_value', 'getRepeatedStringValue', [7]],
            ['repeated_string_value', 'getRepeatedStringValue', [7.7]],
            ['repeated_string_value', 'getRepeatedStringValue', ['seven', 'eight']],
            ['repeated_string_value', 'getRepeatedStringValue', [$sv7, 'eight']],
            ['repeated_string_value', 'getRepeatedStringValue', ['seven', $sv8]],
            ['repeated_string_value', 'getRepeatedStringValue', $repeatedField],
        ];
    }

    /**
     * @dataProvider constructorWithMapWrapperTypeDataProvider
     */
    public function testConstructorWithMapWrapperType($wrapperField, $getter, $value)
    {
        $actualInstance = new TestWrapperSetters([$wrapperField => $value]);
        foreach ($actualInstance->$getter() as $key => $actualWrapperValue) {
            $actualInnerValue = $actualWrapperValue->getValue();
            $expectedElement = $value[$key];
            if (is_object($expectedElement) && is_a($expectedElement, '\Google\Protobuf\StringValue')) {
                $expectedInnerValue = $expectedElement->getValue();
            } elseif (is_object($expectedElement) && is_a($expectedElement, '\Google\Protobuf\Internal\MapEntry')) {
                $expectedInnerValue = $expectedElement->getValue()->getValue();
            } else {
                $expectedInnerValue = $expectedElement;
            }
            $this->assertEquals($expectedInnerValue, $actualInnerValue);
        }

        $this->assertTrue(true);
    }

    public static function constructorWithMapWrapperTypeDataProvider()
    {
        $sv7 = new StringValue(['value' => 'seven']);
        $sv8 = new StringValue(['value' => 'eight']);

        $testWrapperSetters = new TestWrapperSetters();
        $testWrapperSetters->setMapStringValue(['key' => $sv7, 'key2' => $sv8]);
        $mapField = $testWrapperSetters->getMapStringValue();

        return [
            ['map_string_value', 'getMapStringValue', []],
            ['map_string_value', 'getMapStringValue', ['key' => $sv7]],
            ['map_string_value', 'getMapStringValue', ['key' => $sv7, 'key2' => $sv8]],
            ['map_string_value', 'getMapStringValue', ['key' => 'seven']],
            ['map_string_value', 'getMapStringValue', ['key' => 7]],
            ['map_string_value', 'getMapStringValue', ['key' => 7.7]],
            ['map_string_value', 'getMapStringValue', ['key' => 'seven', 'key2' => 'eight']],
            ['map_string_value', 'getMapStringValue', ['key' => $sv7, 'key2' => 'eight']],
            ['map_string_value', 'getMapStringValue', ['key' => 'seven', 'key2' => $sv8]],
            ['map_string_value', 'getMapStringValue', $mapField],
        ];
    }
}
