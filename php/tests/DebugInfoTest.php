<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;
use Foo\EmptyAnySerialization;
use Foo\TestInt32Value;
use Foo\TestStringValue;
use Foo\TestAny;
use Foo\TestEnum;
use Foo\TestLargeFieldNumber;
use Foo\TestMessage;
use Foo\TestMessage\Sub;
use Foo\TestPackedMessage;
use Foo\TestRandomFieldOrder;
use Foo\TestUnpackedMessage;
use Google\Protobuf\Any;
use Google\Protobuf\DoubleValue;
use Google\Protobuf\FieldMask;
use Google\Protobuf\FloatValue;
use Google\Protobuf\Int32Value;
use Google\Protobuf\UInt32Value;
use Google\Protobuf\Int64Value;
use Google\Protobuf\UInt64Value;
use Google\Protobuf\BoolValue;
use Google\Protobuf\StringValue;
use Google\Protobuf\BytesValue;
use Google\Protobuf\Value;
use Google\Protobuf\ListValue;
use Google\Protobuf\Struct;
use Google\Protobuf\GPBEmpty;

class DebugInfoTest extends TestBase
{
    public function setUp(): void
    {
        if (extension_loaded('protobuf')) {
            $this->markTestSkipped('__debugInfo is not supported for the protobuf extension');
        }
    }

    public function testVarDumpOutput()
    {
        $m = new DoubleValue();
        $m->setValue(1.5);
        var_dump($m);
        $regex = <<<EOL
object(Google\Protobuf\DoubleValue)#%s (1) {
  ["value"]=>
  float(1.5)
}
EOL;
        $this->expectOutputRegex('/' . sprintf(preg_quote($regex), '\d+') . '/');
    }

    public function testTopLevelDoubleValue()
    {
        $m = new DoubleValue();
        $m->setValue(1.5);
        $this->assertSame(['value' => 1.5], $m->__debugInfo());
    }

    public function testTopLevelFloatValue()
    {
        $m = new FloatValue();
        $m->setValue(1.5);
        $this->assertSame(['value' => 1.5], $m->__debugInfo());
    }

    public function testTopLevelInt32Value()
    {
        $m = new Int32Value();
        $m->setValue(1);
        $this->assertSame(['value' => 1], $m->__debugInfo());
    }

    public function testTopLevelUInt32Value()
    {
        $m = new UInt32Value();
        $m->setValue(1);
        $this->assertSame(['value' => 1], $m->__debugInfo());
    }

    public function testTopLevelInt64Value()
    {
        $m = new Int64Value();
        $m->setValue(1);
        $this->assertSame(['value' => '1'], $m->__debugInfo());
    }

    public function testTopLevelUInt64Value()
    {
        $m = new UInt64Value();
        $m->setValue(1);
        $this->assertSame(['value' => '1'], $m->__debugInfo());
    }

    public function testTopLevelStringValue()
    {
        $m = new StringValue();
        $m->setValue("a");
        $this->assertSame(['value' => 'a'], $m->__debugInfo());
    }

    public function testTopLevelBoolValue()
    {
        $m = new BoolValue();
        $m->setValue(true);
        $this->assertSame(['value' => true], $m->__debugInfo());
    }

    public function testTopLevelBytesValue()
    {
        $m = new BytesValue();
        $m->setValue("a");
        $this->assertSame(['value' => 'YQ=='], $m->__debugInfo());
    }

    public function testTopLevelLongBytesValue()
    {
        $m = new BytesValue();
        $data = $this->generateRandomString(12007);
        $m->setValue($data);
        $expected = base64_encode($data);
        $this->assertSame(['value' => $expected], $m->__debugInfo());
    }

    public function testJsonEncodeNullSubMessage()
    {
        $from = new TestMessage();
        $from->setOptionalMessage(null);
        $data = $from->__debugInfo();
        $this->assertEquals([], $data);
    }

    public function testDuration()
    {
        $m = new Google\Protobuf\Duration();
        $m->setSeconds(1234);
        $m->setNanos(999999999);
        $this->assertEquals([
            'seconds' => 1234,
            'nanos' => 999999999,
        ], $m->__debugInfo());
    }

    public function testTimestamp()
    {
        $m = new Google\Protobuf\Timestamp();
        $m->setSeconds(946684800);
        $m->setNanos(123456789);
        $this->assertEquals([
            'seconds' => 946684800,
            'nanos' => 123456789,
        ], $m->__debugInfo());
    }

    public function testTopLevelValue()
    {
        $m = new Value();
        $m->setStringValue("a");
        $this->assertSame(['stringValue' => 'a'], $m->__debugInfo());

        $m = new Value();
        $m->setNumberValue(1.5);
        $this->assertSame(['numberValue' => 1.5], $m->__debugInfo());

        $m = new Value();
        $m->setBoolValue(true);
        $this->assertSame(['boolValue' => true], $m->__debugInfo());

        $m = new Value();
        $m->setNullValue(\Google\Protobuf\NullValue::NULL_VALUE);
        $this->assertSame(['nullValue' => 0], $m->__debugInfo());

        $m = new Value();
        $struct = new Struct();
        $map = $struct->getFields();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $map["a"] = $sub;
        $m->setStructValue($struct);
        $this->assertSame(['structValue' => ['a' => 1.5]], $m->__debugInfo());

        $m = new Value();
        $list = new ListValue();
        $arr = $list->getValues();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $arr[] = $sub;
        $m->setListValue($list);
        $this->assertSame(['listValue' => [1.5]], $m->__debugInfo());
    }

    public function testTopLevelListValue()
    {
        $m = new ListValue();
        $arr = $m->getValues();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $arr[] = $sub;
        $this->assertSame([1.5], $m->__debugInfo());
    }

    public function testEmptyListValue()
    {
        $m = new Struct();
        $m->setFields(['test' => (new Value())->setListValue(new ListValue())]);
        $this->assertSame(['test' => []], $m->__debugInfo());
    }

    public function testTopLevelStruct()
    {
        $m = new Struct();
        $map = $m->getFields();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $map["a"] = $sub;
        $this->assertSame(['a' => 1.5], $m->__debugInfo());
    }

    public function testEmptyStruct()
    {
        $m = new Struct();
        $m->setFields(['test' => (new Value())->setStructValue(new Struct())]);
        $this->assertSame(['test' => []], $m->__debugInfo());
    }

    public function testEmptyValue()
    {
        $m = new Value();
        $this->assertSame([], $m->__debugInfo());
    }

    public function testTopLevelAny()
    {
        // Test a normal message.
        $packed = new TestMessage();
        $packed->setOptionalInt32(123);
        $packed->setOptionalString("abc");

        $m = new Any();
        $m->pack($packed);
        $expected = [
            '@type' => 'type.googleapis.com/foo.TestMessage',
            'optionalInt32' => 123,
            'optionalString' => 'abc',
        ];
        $result = $m->__debugInfo();
        $this->assertSame($expected, $result);

        // Test a well known message.
        $packed = new Int32Value();
        $packed->setValue(123);

        $m = new Any();
        $m->pack($packed);
        $this->assertSame([
            '@type' => 'type.googleapis.com/google.protobuf.Int32Value',
            'value' =>  123
        ], $m->__debugInfo());

        // Test an Any message.
        $outer = new Any();
        $outer->pack($m);
        $this->assertSame([
            '@type' => 'type.googleapis.com/google.protobuf.Any',
            'value' =>  [
                '@type' => 'type.googleapis.com/google.protobuf.Int32Value',
                'value' => 123
            ],
        ], $outer->__debugInfo());

        // Test a Timestamp message.
        $packed = new Google\Protobuf\Timestamp();
        $packed->setSeconds(946684800);
        $packed->setNanos(123456789);
        $m = new Any();
        $m->pack($packed);
        $this->assertSame([
            '@type' => 'type.googleapis.com/google.protobuf.Timestamp',
            'value' =>  '2000-01-01T00:00:00.123456789Z',
        ], $m->__debugInfo());
    }

    public function testAnyWithDefaultWrapperMessagePacked()
    {
        $any = new Any();
        $any->pack(new TestInt32Value([
            'field' => new Int32Value(['value' => 0]),
        ]));
        $this->assertSame(
            ['@type' => 'type.googleapis.com/foo.TestInt32Value', 'field' => 0],
            $any->__debugInfo()
        );
    }

    public function testTopLevelFieldMask()
    {
        $m = new FieldMask();
        $m->setPaths(["foo.bar_baz", "qux"]);
        $this->assertSame(['paths' => ['foo.bar_baz', 'qux']], $m->__debugInfo());
    }

    public function testEmptyAnySerialization()
    {
        $m = new EmptyAnySerialization();

        $any = new Any();
        $any->pack($m);

        $data = $any->__debugInfo();
        $this->assertEquals(['@type' => 'type.googleapis.com/foo.EmptyAnySerialization'], $data);
    }

    public function testRepeatedStringValue()
    {
        $m = new TestStringValue();
        $r = [new StringValue(['value' => 'a'])];
        $m->setRepeatedField($r);
        $this->assertSame(['repeatedField' => ['a']], $m->__debugInfo());
    }

    public function testMapStringValue()
    {
        $m = new TestStringValue();
        $m->mergeFromJsonString("{\"map_field\":{\"1\": \"a\"}}");
        $this->assertSame(['mapField' => [1 => 'a']], $m->__debugInfo());
    }

    public function testNestedAny()
    {
        // Make sure packed message has been created at least once.
        $m = new TestAny();
        $m->mergeFromJsonString(
            "{\"any\":{\"optionalInt32\": 1, " .
            "\"@type\":\"type.googleapis.com/foo.TestMessage\", " .
            "\"optionalInt64\": 2}}");
        $this->assertSame([
            'any' => [
                '@type' => 'type.googleapis.com/foo.TestMessage',
                'optionalInt32' => 1,
                'optionalInt64' => '2',
            ]
        ], $m->__debugInfo());
    }

    public function testEnum()
    {
        $m = new TestMessage();
        $m->setOneofEnum(TestEnum::ZERO);
        $this->assertSame(['oneofEnum' => 'ZERO'], $m->__debugInfo());
    }

    public function testTopLevelRepeatedField()
    {
        $r1 = new RepeatedField(GPBType::class);
        $r1[] = 'a';
        $this->assertSame(['a'], $r1->__debugInfo());

        $r2 = new RepeatedField(TestMessage::class);
        $r2[] = new TestMessage(['optional_int32' => -42]);
        $r2[] = new TestMessage(['optional_int64' => 43]);
        $this->assertSame([
            ['optionalInt32' => -42],
            ['optionalInt64' => '43'],
        ], $r2->__debugInfo());

        $r3 = new RepeatedField(RepeatedField::class);
        $r3[] = $r1;
        $r3[] = $r2;

        $this->assertSame([
            ['a'],
            [
                ['optionalInt32' => -42],
                ['optionalInt64' => '43'],
            ],
        ], $r3->__debugInfo());
    }

    public function testEverything()
    {
        $m = new TestMessage([
            'optional_int32' => -42,
            'optional_int64' => -43,
            'optional_uint32' => 42,
            'optional_uint64' => 43,
            'optional_sint32' => -44,
            'optional_sint64' => -45,
            'optional_fixed32' => 46,
            'optional_fixed64' => 47,
            'optional_sfixed32' => -46,
            'optional_sfixed64' => -47,
            'optional_float' => 1.5,
            'optional_double' => 1.6,
            'optional_bool' => true,
            'optional_string' => 'a',
            'optional_bytes' => 'bbbb',
            'optional_enum' => TestEnum::ONE,
            'optional_message' => new Sub([
                'a' => 33
            ]),
            'repeated_int32' => [-42, -52],
            'repeated_int64' => [-43, -53],
            'repeated_uint32' => [42, 52],
            'repeated_uint64' => [43, 53],
            'repeated_sint32' => [-44, -54],
            'repeated_sint64' => [-45, -55],
            'repeated_fixed32' => [46, 56],
            'repeated_fixed64' => [47, 57],
            'repeated_sfixed32' => [-46, -56],
            'repeated_sfixed64' => [-47, -57],
            'repeated_float' => [1.5, 2.5],
            'repeated_double' => [1.6, 2.6],
            'repeated_bool' => [true, false],
            'repeated_string' => ['a', 'c'],
            'repeated_bytes' => ['bbbb', 'dddd'],
            'repeated_enum' => [TestEnum::ZERO, TestEnum::ONE],
            'repeated_message' => [new Sub(['a' => 34]),
                                   new Sub(['a' => 35])],
            'map_int32_int32' => [-62 => -62],
            'map_int64_int64' => [-63 => -63],
            'map_uint32_uint32' => [62 => 62],
            'map_uint64_uint64' => [63 => 63],
            'map_sint32_sint32' => [-64 => -64],
            'map_sint64_sint64' => [-65 => -65],
            'map_fixed32_fixed32' => [66 => 66],
            'map_fixed64_fixed64' => [67 => 67],
            'map_sfixed32_sfixed32' => [-68 => -68],
            'map_sfixed64_sfixed64' => [-69 => -69],
            'map_int32_float' => [1 => 3.5],
            'map_int32_double' => [1 => 3.6],
            'map_bool_bool' => [true => true],
            'map_string_string' => ['e' => 'e'],
            'map_int32_bytes' => [1 => 'ffff'],
            'map_int32_enum' => [1 => TestEnum::ONE],
            'map_int32_message' => [1 => new Sub(['a' => 36])],
        ]);

        $this->assertSame([
            'optionalInt32' => -42,
            'optionalInt64' => '-43',
            'optionalUint32' => 42,
            'optionalUint64' => '43',
            'optionalSint32' => -44,
            'optionalSint64' => '-45',
            'optionalFixed32' => 46,
            'optionalFixed64' => '47',
            'optionalSfixed32' => -46,
            'optionalSfixed64' => '-47',
            'optionalFloat' => 1.5,
            'optionalDouble' => 1.6,
            'optionalBool' => true,
            'optionalString' => 'a',
            'optionalBytes' => 'YmJiYg==',
            'optionalEnum' => 'ONE',
            'optionalMessage' => [
                'a' => 33,
            ],
            'repeatedInt32' => [
                -42,
                -52,
            ],
            'repeatedInt64' => [
                '-43',
                '-53',
            ],
            'repeatedUint32' => [
                42,
                52,
            ],
            'repeatedUint64' => [
                '43',
                '53',
            ],
            'repeatedSint32' => [
                -44,
                -54,
            ],
            'repeatedSint64' => [
                '-45',
                '-55',
            ],
            'repeatedFixed32' => [
                46,
                56,
            ],
            'repeatedFixed64' => [
                '47',
                '57',
            ],
            'repeatedSfixed32' => [
                -46,
                -56,
            ],
            'repeatedSfixed64' => [
                '-47',
                '-57',
            ],
            'repeatedFloat' => [
                1.5,
                2.5,
            ],
            'repeatedDouble' => [
                1.6,
                2.6,
            ],
            'repeatedBool' => [
                true,
                false,
            ],
            'repeatedString' => [
                'a',
                'c',
            ],
            'repeatedBytes' => [
                'YmJiYg==',
                'ZGRkZA==',
            ],
            'repeatedEnum' => [
                'ZERO',
                'ONE',
            ],
            'repeatedMessage' => [
                [
                    'a' => 34,
                ],
                [
                    'a' => 35,
                ],
            ],
            'mapInt32Int32' => [
                -62 => -62,
            ],
            'mapInt64Int64' => [
                -63 => '-63',
            ],
            'mapUint32Uint32' => [
                62 => 62,
            ],
            'mapUint64Uint64' => [
                63 => '63',
            ],
            'mapSint32Sint32' => [
                -64 => -64,
            ],
            'mapSint64Sint64' => [
                -65 => '-65',
            ],
            'mapFixed32Fixed32' => [
                66 => 66,
            ],
            'mapFixed64Fixed64' => [
                67 => '67',
            ],
            'mapSfixed32Sfixed32' => [
                -68 => -68,
            ],
            'mapSfixed64Sfixed64' => [
                -69 => '-69',
            ],
            'mapInt32Float' => [
                1 => 3.5,
            ],
            'mapInt32Double' => [
                1 => 3.6,
            ],
            'mapBoolBool' => [
                'true' => true,
            ],
            'mapStringString' => [
                'e' => 'e',
            ],
            'mapInt32Bytes' => [
                1 => 'ZmZmZg==',
            ],
            'mapInt32Enum' => [
                1 => 'ONE',
            ],
            'mapInt32Message' => [
                1 => ['a' => 36],
            ],
        ], $m->__debugInfo());
    }

    private function generateRandomString($length = 10)
    {
        $randomString = str_repeat("+", $length);
        for ($i = 0; $i < $length; $i++) {
            $randomString[$i] = chr(rand(0, 255));
        }
        return $randomString;
    }
}
