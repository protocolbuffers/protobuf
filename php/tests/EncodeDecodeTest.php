<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\RepeatedField;
use Google\Protobuf\GPBType;
use Foo\TestInt32Value;
use Foo\TestInt64Value;
use Foo\TestUInt32Value;
use Foo\TestUInt64Value;
use Foo\TestBoolValue;
use Foo\TestStringValue;
use Foo\TestBytesValue;
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

class EncodeDecodeTest extends TestBase
{
    public function testDecodeJsonSimple()
    {
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"optionalInt32\":1}");
        $this->assertEquals(1, $m->getOptionalInt32());
    }

    public function testDecodeJsonUnknown()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":1}");
    }

    public function testDecodeJsonIgnoreUnknown()
    {
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":1}", true);
        $this->assertEquals("{}", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelBoolValue()
    {
        $m = new BoolValue();

        $m->mergeFromJsonString("true");
        $this->assertEquals(true, $m->getValue());

        $m->mergeFromJsonString("false");
        $this->assertEquals(false, $m->getValue());
    }

    public function testEncodeTopLevelBoolValue()
    {
        $m = new BoolValue();
        $m->setValue(true);
        $this->assertSame("true", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelDoubleValue()
    {
        $m = new DoubleValue();
        $m->mergeFromJsonString("1.5");
        $this->assertEquals(1.5, $m->getValue());
    }

    public function testEncodeTopLevelDoubleValue()
    {
        $m = new DoubleValue();
        $m->setValue(1.5);
        $this->assertSame("1.5", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelFloatValue()
    {
        $m = new FloatValue();
        $m->mergeFromJsonString("1.5");
        $this->assertEquals(1.5, $m->getValue());
    }

    public function testEncodeTopLevelFloatValue()
    {
        $m = new FloatValue();
        $m->setValue(1.5);
        $this->assertSame("1.5", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelInt32Value()
    {
        $m = new Int32Value();
        $m->mergeFromJsonString("1");
        $this->assertEquals(1, $m->getValue());
    }

    public function testEncodeTopLevelInt32Value()
    {
        $m = new Int32Value();
        $m->setValue(1);
        $this->assertSame("1", $m->serializeToJsonString());
    }

    public function testDecodeRepeatedInt32Value()
    {
        $m = new TestInt32Value();
        $m->mergeFromJsonString("{\"repeated_field\":[12345]}");
        $this->assertSame(12345, $m->getRepeatedField()[0]->getValue());
    }

    public function testDecodeTopLevelUInt32Value()
    {
        $m = new UInt32Value();
        $m->mergeFromJsonString("1");
        $this->assertEquals(1, $m->getValue());
    }

    public function testEncodeTopLevelUInt32Value()
    {
        $m = new UInt32Value();
        $m->setValue(1);
        $this->assertSame("1", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelInt64Value()
    {
        $m = new Int64Value();
        $m->mergeFromJsonString("1");
        $this->assertEquals(1, $m->getValue());
    }

    public function testDecodeTopLevelInt64ValueAsString()
    {
        $m = new Int64Value();
        $m->mergeFromJsonString("\"1\"");
        $this->assertEquals(1, $m->getValue());
    }

    public function testEncodeTopLevelInt64Value()
    {
        $m = new Int64Value();
        $m->setValue(1);
        $this->assertSame("\"1\"", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelUInt64Value()
    {
        $m = new UInt64Value();
        $m->mergeFromJsonString("1");
        $this->assertEquals(1, $m->getValue());
    }

    public function testDecodeTopLevelUInt64ValueAsString()
    {
        $m = new UInt64Value();
        $m->mergeFromJsonString("\"1\"");
        $this->assertEquals(1, $m->getValue());
    }

    public function testEncodeTopLevelUInt64Value()
    {
        $m = new UInt64Value();
        $m->setValue(1);
        $this->assertSame("\"1\"", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelStringValue()
    {
        $m = new StringValue();
        $m->mergeFromJsonString("\"a\"");
        $this->assertSame("a", $m->getValue());
    }

    public function testEncodeTopLevelStringValue()
    {
        $m = new StringValue();
        $m->setValue("a");
        $this->assertSame("\"a\"", $m->serializeToJsonString());
    }

    public function testDecodeRepeatedStringValue()
    {
        $m = new TestStringValue();
        $m->mergeFromJsonString("{\"repeated_field\":[\"a\"]}");
        $this->assertSame("a", $m->getRepeatedField()[0]->getValue());
    }

    public function testDecodeMapStringValue()
    {
        $m = new TestStringValue();
        $m->mergeFromJsonString("{\"map_field\":{\"1\": \"a\"}}");
        $this->assertSame("a", $m->getMapField()[1]->getValue());
    }

    public function testDecodeTopLevelBytesValue()
    {
        $m = new BytesValue();
        $m->mergeFromJsonString("\"YQ==\"");
        $this->assertSame("a", $m->getValue());
    }

    public function testEncodeTopLevelBytesValue()
    {
        $m = new BytesValue();
        $m->setValue("a");
        $this->assertSame("\"YQ==\"", $m->serializeToJsonString());
    }

    public function generateRandomString($length = 10) {
        $randomString = str_repeat("+", $length);
        for ($i = 0; $i < $length; $i++) {
            $randomString[$i] = chr(rand(0, 255));
        }
        return $randomString;
    }

    public function testEncodeTopLevelLongBytesValue()
    {
        $m = new BytesValue();
        $data = $this->generateRandomString(12007);
        $m->setValue($data);
        $expected = "\"" . base64_encode($data) . "\"";
        $this->assertSame(strlen($expected), strlen($m->serializeToJsonString()));
    }

    public function testEncode()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);
        $this->setFields($from);
        $this->expectFields($from);

        $data = $from->serializeToString();
        $this->assertSame(bin2hex(TestUtil::getGoldenTestMessage()),
                          bin2hex($data));
    }

    public function testDecode()
    {
        $to = new TestMessage();
        $to->mergeFromString(TestUtil::getGoldenTestMessage());
        $this->expectFields($to);
    }

    public function testEncodeDecode()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);
        $this->setFields($from);
        $this->expectFields($from);

        $data = $from->serializeToString();

        $to = new TestMessage();
        $to->mergeFromString($data);
        $this->expectFields($to);
    }

    public function testEncodeDecodeEmpty()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);

        $data = $from->serializeToString();

        $to = new TestMessage();
        $to->mergeFromString($data);
        $this->expectEmptyFields($to);
    }

    public function testEncodeDecodeOneof()
    {
        $m = new TestMessage();

        $m->setOneofInt32(1);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame(1, $n->getOneofInt32());

        $m->setOneofFloat(2.0);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame(2.0, $n->getOneofFloat());

        $m->setOneofString('abc');
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame('abc', $n->getOneofString());

        $sub_m = new Sub();
        $sub_m->setA(1);
        $m->setOneofMessage($sub_m);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame(1, $n->getOneofMessage()->getA());

        // Encode default value
        $m->setOneofEnum(TestEnum::ZERO);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame("oneof_enum", $n->getMyOneof());
        $this->assertSame(TestEnum::ZERO, $n->getOneofEnum());

        $m->setOneofString("");
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame("oneof_string", $n->getMyOneof());
        $this->assertSame("", $n->getOneofString());

        $sub_m = new Sub();
        $m->setOneofMessage($sub_m);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
        $this->assertSame("oneof_message", $n->getMyOneof());
        $this->assertFalse(is_null($n->getOneofMessage()));

    }

    public function testEncodeDecodeOptional()
    {
      $m = new TestMessage();
      $this->assertFalse($m->hasTrueOptionalInt32());
      $data = $m->serializeToString();
      $this->assertSame("", $data);

      $m->setTrueOptionalInt32(0);
      $this->assertTrue($m->hasTrueOptionalInt32());
      $data = $m->serializeToString();
      $this->assertNotSame("", $data);

      $m2 = new TestMessage();
      $m2->mergeFromString($data);
      $this->assertTrue($m2->hasTrueOptionalInt32());
      $this->assertSame(0, $m2->getTrueOptionalInt32());
    }

    public function testJsonEncodeDecodeOptional()
    {
      $m = new TestMessage();
      $this->assertFalse($m->hasTrueOptionalInt32());
      $data = $m->serializeToJsonString();
      $this->assertSame("{}", $data);

      $m->setTrueOptionalInt32(0);
      $this->assertTrue($m->hasTrueOptionalInt32());
      $data = $m->serializeToJsonString();
      $this->assertNotSame("{}", $data);

      $m2 = new TestMessage();
      $m2->mergeFromJsonString($data);
      $this->assertTrue($m2->hasTrueOptionalInt32());
      $this->assertSame(0, $m2->getTrueOptionalInt32());
    }

    public function testJsonEncodeDecodeOneof()
    {
        $m = new TestMessage();

        $m->setOneofEnum(TestEnum::ONE);
        $data = $m->serializeToJsonString();
        $n = new TestMessage();
        $n->mergeFromJsonString($data);
        $this->assertSame("oneof_enum", $n->getMyOneof());
        $this->assertSame(TestEnum::ONE, $n->getOneofEnum());

        $m->setOneofString("a");
        $data = $m->serializeToJsonString();
        $n = new TestMessage();
        $n->mergeFromJsonString($data);
        $this->assertSame("oneof_string", $n->getMyOneof());
        $this->assertSame("a", $n->getOneofString());

        $m->setOneofBytes("bbbb");
        $data = $m->serializeToJsonString();
        $n = new TestMessage();
        $n->mergeFromJsonString($data);
        $this->assertSame("oneof_bytes", $n->getMyOneof());
        $this->assertSame("bbbb", $n->getOneofBytes());

        $sub_m = new Sub();
        $m->setOneofMessage($sub_m);
        $data = $m->serializeToJsonString();
        $n = new TestMessage();
        $n->mergeFromJsonString($data);
        $this->assertSame("oneof_message", $n->getMyOneof());
        $this->assertFalse(is_null($n->getOneofMessage()));
    }

    public function testPackedEncode()
    {
        $from = new TestPackedMessage();
        TestUtil::setTestPackedMessage($from);
        $this->assertSame(TestUtil::getGoldenTestPackedMessage(),
                          $from->serializeToString());
    }

    public function testPackedDecodePacked()
    {
        $to = new TestPackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestPackedMessage());
        TestUtil::assertTestPackedMessage($to);
        $this->assertTrue(true);
    }

    public function testPackedDecodeUnpacked()
    {
        $to = new TestPackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
        $this->assertTrue(true);
    }

    public function testUnpackedEncode()
    {
        $from = new TestUnpackedMessage();
        TestUtil::setTestPackedMessage($from);
        $this->assertSame(TestUtil::getGoldenTestUnpackedMessage(),
                          $from->serializeToString());
    }

    public function testUnpackedDecodePacked()
    {
        $to = new TestUnpackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestPackedMessage());
        TestUtil::assertTestPackedMessage($to);
        $this->assertTrue(true);
    }

    public function testUnpackedDecodeUnpacked()
    {
        $to = new TestUnpackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
        $this->assertTrue(true);
    }

    public function testDecodeInt64()
    {
        // Read 64 testing
        $testVals = array(
            '10'                 => '100a',
            '100'                => '1064',
            '800'                => '10a006',
            '6400'               => '108032',
            '70400'              => '1080a604',
            '774400'             => '1080a22f',
            '9292800'            => '108098b704',
            '74342400'           => '1080c0b923',
            '743424000'          => '108080bfe202',
            '8177664000'         => '108080b5bb1e',
            '65421312000'        => '108080a8dbf301',
            '785055744000'       => '108080e0c7ec16',
            '9420668928000'      => '10808080dd969202',
            '103627358208000'    => '10808080fff9c717',
            '1139900940288000'   => '10808080f5bd978302',
            '13678811283456000'  => '10808080fce699a618',
            '109430490267648000' => '10808080e0b7ceb1c201',
            '984874412408832000' => '10808080e0f5c1bed50d',
        );

        $msg = new TestMessage();
        foreach ($testVals as $original => $encoded) {
            $msg->setOptionalInt64($original);
            $data = $msg->serializeToString();
            $this->assertSame($encoded, bin2hex($data));
            $msg->setOptionalInt64(0);
            $msg->mergeFromString($data);
            $this->assertEquals($original, $msg->getOptionalInt64());
        }
    }

    public function testDecodeToExistingMessage()
    {
        $m1 = new TestMessage();
        $this->setFields($m1);
        $this->expectFields($m1);

        $m2 = new TestMessage();
        $this->setFields2($m2);
        $data = $m2->serializeToString();

        $m1->mergeFromString($data);
        $this->expectFieldsMerged($m1);
    }

    public function testDecodeFieldNonExist()
    {
        $data = hex2bin('c80501');
        $m = new TestMessage();
        $m->mergeFromString($data);
        $this->assertTrue(true);
    }

    public function testEncodeNegativeInt32()
    {
        $m = new TestMessage();
        $m->setOptionalInt32(-1);
        $data = $m->serializeToString();
        $this->assertSame("08ffffffffffffffffff01", bin2hex($data));
    }

    public function testDecodeNegativeInt32()
    {
        $m = new TestMessage();
        $this->assertEquals(0, $m->getOptionalInt32());
        $m->mergeFromString(hex2bin("08ffffffffffffffffff01"));
        $this->assertEquals(-1, $m->getOptionalInt32());

        $m = new TestMessage();
        $this->assertEquals(0, $m->getOptionalInt32());
        $m->mergeFromString(hex2bin("08ffffffff0f"));
        $this->assertEquals(-1, $m->getOptionalInt32());
    }

    public function testRandomFieldOrder()
    {
        $m = new TestRandomFieldOrder();
        $data = $m->serializeToString();
        $this->assertSame("", $data);
    }

    public function testLargeFieldNumber()
    {
        $m = new TestLargeFieldNumber(['large_field_number' => 5]);
        $data = $m->serializeToString();
        $m2 = new TestLargeFieldNumber();
        $m2->mergeFromString($data);
        $this->assertSame(5, $m2->getLargeFieldNumber());
    }

    public function testDecodeInvalidInt32()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('08'));
    }

    public function testDecodeInvalidSubMessage()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('9A010108'));
    }

    public function testDecodeInvalidInt64()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('10'));
    }

    public function testDecodeInvalidUInt32()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('18'));
    }

    public function testDecodeInvalidUInt64()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('20'));
    }

    public function testDecodeInvalidSInt32()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('28'));
    }

    public function testDecodeInvalidSInt64()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('30'));
    }

    public function testDecodeInvalidFixed32()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('3D'));
    }

    public function testDecodeInvalidFixed64()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('41'));
    }

    public function testDecodeInvalidSFixed32()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('4D'));
    }

    public function testDecodeInvalidSFixed64()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('51'));
    }

    public function testDecodeInvalidFloat()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('5D'));
    }

    public function testDecodeInvalidDouble()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('61'));
    }

    public function testDecodeInvalidBool()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('68'));
    }

    public function testDecodeInvalidStringLengthMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('72'));
    }

    public function testDecodeInvalidStringDataMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7201'));
    }

    public function testDecodeInvalidBytesLengthMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7A'));
    }

    public function testDecodeInvalidBytesDataMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7A01'));
    }

    public function testDecodeInvalidEnum()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8001'));
    }

    public function testDecodeInvalidMessageLengthMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8A01'));
    }

    public function testDecodeInvalidMessageDataMiss()
    {
        $this->expectException(Exception::class);

        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8A0101'));
    }

    public function testDecodeInvalidPackedMessageLength()
    {
        $this->expectException(Exception::class);

        $m = new TestPackedMessage();
        $m->mergeFromString(hex2bin('D205'));
    }

    public function testUnknown()
    {
        // Test preserve unknown for varint.
        $m = new TestMessage();
        $from = hex2bin('F80601');  // TODO(teboring): Add a util to encode
                                    // varint for better readability
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        // Test preserve unknown for 64-bit.
        $m = new TestMessage();
        $from = hex2bin('F9060000000000000000');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        // Test preserve unknown for length delimited.
        $m = new TestMessage();
        $from = hex2bin('FA0600');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        // Test preserve unknown for 32-bit.
        $m = new TestMessage();
        $from = hex2bin('FD0600000000');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        // Test discard unknown in message.
        $m = new TestMessage();
        $from = hex2bin('F80601');
        $m->mergeFromString($from);
        $m->discardUnknownFields();
        $to = $m->serializeToString();
        $this->assertSame("", bin2hex($to));

        // Test discard unknown for singular message field.
        $m = new TestMessage();
        $from = hex2bin('8A0103F80601');
        $m->mergeFromString($from);
        $m->discardUnknownFields();
        $to = $m->serializeToString();
        $this->assertSame("8a0100", bin2hex($to));

        // Test discard unknown for repeated message field.
        $m = new TestMessage();
        $from = hex2bin('FA0203F80601');
        $m->mergeFromString($from);
        $m->discardUnknownFields();
        $to = $m->serializeToString();
        $this->assertSame("fa0200", bin2hex($to));

        // Test discard unknown for map message value field.
        $m = new TestMessage();
        $from = hex2bin("BA050708011203F80601");
        $m->mergeFromString($from);
        $m->discardUnknownFields();
        $to = $m->serializeToString();
        $this->assertSame("ba050408011200", bin2hex($to));

        // Test discard unknown for singular message field.
        $m = new TestMessage();
        $from = hex2bin('9A0403F80601');
        $m->mergeFromString($from);
        $m->discardUnknownFields();
        $to = $m->serializeToString();
        $this->assertSame("9a0400", bin2hex($to));
    }

    public function testJsonUnknown()
    {
        // Test unknown number
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":1,
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown bool
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":true,
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown string
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":\"abc\",
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown null
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":null,
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown number array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[1],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown bool array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[true],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown string array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[\"a\"],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown null array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[null],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown array array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[[]],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown object array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[{}],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown double value array
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":[1, 2],
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown number object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":1},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown bool object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":true},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown string object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":\"a\"},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown null object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":null},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown array object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":[]},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown object object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":{}},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown double value object
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"unknown\":{\"a\":1, \"b\":1},
                                \"optionalInt32\":1}", true);
        $this->assertSame(1, $m->getOptionalInt32());

        // Test unknown enum value
        $m = new TestMessage();
        $m->mergeFromJsonString("{\"optionalEnum\":\"UNKNOWN\"}", true);
        $this->assertSame(0, $m->getOptionalEnum());
    }

    public function testJsonEncode()
    {
        $from = new TestMessage();
        $this->setFields($from);
        $data = $from->serializeToJsonString();
        $to = new TestMessage();
        $to->mergeFromJsonString($data);
        $this->expectFields($to);
    }

    public function testJsonEncodeNullSubMessage()
    {
        $from = new TestMessage();
        $from->setOptionalMessage(null);
        $data = $from->serializeToJsonString();
        $this->assertEquals("{}", $data);
    }

    public function testDecodeDuration()
    {
        $m = new Google\Protobuf\Duration();
        $m->mergeFromJsonString("\"1234.5678s\"");
        $this->assertEquals(1234, $m->getSeconds());
        $this->assertEquals(567800000, $m->getNanos());
    }

    public function testEncodeDuration()
    {
        $m = new Google\Protobuf\Duration();
        $m->setSeconds(1234);
        $m->setNanos(999999999);
        $this->assertEquals("\"1234.999999999s\"", $m->serializeToJsonString());
    }

    public function testDecodeTimestamp()
    {
        $m = new Google\Protobuf\Timestamp();
        $m->mergeFromJsonString("\"2000-01-01T00:00:00.123456789Z\"");
        $this->assertEquals(946684800, $m->getSeconds());
        $this->assertEquals(123456789, $m->getNanos());
    }

    public function testEncodeTimestamp()
    {
        $m = new Google\Protobuf\Timestamp();
        $m->setSeconds(946684800);
        $m->setNanos(123456789);
        $this->assertEquals("\"2000-01-01T00:00:00.123456789Z\"",
                            $m->serializeToJsonString());
    }

    public function testDecodeTopLevelValue()
    {
        $m = new Value();
        $m->mergeFromJsonString("\"a\"");
        $this->assertSame("a", $m->getStringValue());

        $m = new Value();
        $m->mergeFromJsonString("1.5");
        $this->assertSame(1.5, $m->getNumberValue());

        $m = new Value();
        $m->mergeFromJsonString("true");
        $this->assertSame(true, $m->getBoolValue());

        $m = new Value();
        $m->mergeFromJsonString("null");
        $this->assertSame("null_value", $m->getKind());

        $m = new Value();
        $m->mergeFromJsonString("[1]");
        $this->assertSame("list_value", $m->getKind());

        $m = new Value();
        $m->mergeFromJsonString("{\"a\":1}");
        $this->assertSame("struct_value", $m->getKind());
    }

    public function testEncodeTopLevelValue()
    {
        $m = new Value();
        $m->setStringValue("a");
        $this->assertSame("\"a\"", $m->serializeToJsonString());

        $m = new Value();
        $m->setNumberValue(1.5);
        $this->assertSame("1.5", $m->serializeToJsonString());

        $m = new Value();
        $m->setBoolValue(true);
        $this->assertSame("true", $m->serializeToJsonString());

        $m = new Value();
        $m->setNullValue(0);
        $this->assertSame("null", $m->serializeToJsonString());
    }

    public function testDecodeTopLevelListValue()
    {
        $m = new ListValue();
        $m->mergeFromJsonString("[1]");
        $this->assertSame(1.0, $m->getValues()[0]->getNumberValue());
    }

    public function testEncodeTopLevelListValue()
    {
        $m = new ListValue();
        $arr = $m->getValues();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $arr[] = $sub;
        $this->assertSame("[1.5]", $m->serializeToJsonString());
    }

    public function testEncodeEmptyListValue()
    {
        $m = new Struct();
        $m->setFields(['test' => (new Value())->setListValue(new ListValue())]);
        $this->assertSame('{"test":[]}', $m->serializeToJsonString());
    }

    public function testDecodeTopLevelStruct()
    {
        $m = new Struct();
        $m->mergeFromJsonString("{\"a\":{\"b\":1}}");
        $this->assertSame(1.0, $m->getFields()["a"]
                                 ->getStructValue()
                                 ->getFields()["b"]->getNumberValue());
    }

    public function testEncodeTopLevelStruct()
    {
        $m = new Struct();
        $map = $m->getFields();
        $sub = new Value();
        $sub->setNumberValue(1.5);
        $map["a"] = $sub;
        $this->assertSame("{\"a\":1.5}", $m->serializeToJsonString());
    }

    public function testEncodeEmptyStruct()
    {
        $m = new Struct();
        $m->setFields(['test' => (new Value())->setStructValue(new Struct())]);
        $this->assertSame('{"test":{}}', $m->serializeToJsonString());
    }

    public function testDecodeTopLevelAny()
    {
        // Make sure packed message has been created at least once.
        $packed = new TestMessage();

        $m1 = new Any();
        $m1->mergeFromJsonString(
            "{\"optionalInt32\": 1, " .
            "\"@type\":\"type.googleapis.com/foo.TestMessage\"}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m1->getTypeUrl());
        $this->assertSame("0801", bin2hex($m1->getValue()));

        $m2 = new Any();
        $m2->mergeFromJsonString(
            "{\"@type\":\"type.googleapis.com/foo.TestMessage\", " .
            "\"optionalInt32\": 1}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m2->getTypeUrl());
        $this->assertSame("0801", bin2hex($m2->getValue()));

        $m3 = new Any();
        $m3->mergeFromJsonString(
            "{\"optionalInt32\": 1, " .
            "\"@type\":\"type.googleapis.com/foo.TestMessage\", " .
            "\"optionalInt64\": 2}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m3->getTypeUrl());
        $this->assertSame("08011002", bin2hex($m3->getValue()));
    }

    public function testDecodeAny()
    {
        // Make sure packed message has been created at least once.
        $packed = new TestMessage();

        $m1 = new TestAny();
        $m1->mergeFromJsonString(
            "{\"any\": {\"optionalInt32\": 1, " .
            "\"@type\":\"type.googleapis.com/foo.TestMessage\"}}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m1->getAny()->getTypeUrl());
        $this->assertSame("0801", bin2hex($m1->getAny()->getValue()));

        $m2 = new TestAny();
        $m2->mergeFromJsonString(
            "{\"any\":{\"@type\":\"type.googleapis.com/foo.TestMessage\", " .
            "\"optionalInt32\": 1}}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m2->getAny()->getTypeUrl());
        $this->assertSame("0801", bin2hex($m2->getAny()->getValue()));

        $m3 = new TestAny();
        $m3->mergeFromJsonString(
            "{\"any\":{\"optionalInt32\": 1, " .
            "\"@type\":\"type.googleapis.com/foo.TestMessage\", " .
            "\"optionalInt64\": 2}}");
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $m3->getAny()->getTypeUrl());
        $this->assertSame("08011002", bin2hex($m3->getAny()->getValue()));
    }

    public function testDecodeAnyWithWellKnownPacked()
    {
        // Make sure packed message has been created at least once.
        $packed = new Int32Value();

        $m1 = new TestAny();
        $m1->mergeFromJsonString(
            "{\"any\":" .
            "  {\"@type\":\"type.googleapis.com/google.protobuf.Int32Value\"," .
            "   \"value\":1}}");
        $this->assertSame("type.googleapis.com/google.protobuf.Int32Value",
                          $m1->getAny()->getTypeUrl());
        $this->assertSame("0801", bin2hex($m1->getAny()->getValue()));
    }

    public function testDecodeAnyWithUnknownPacked()
    {
        $this->expectException(Exception::class);

        $m = new TestAny();
        $m->mergeFromJsonString(
            "{\"any\":" .
            "  {\"@type\":\"type.googleapis.com/unknown\"," .
            "   \"value\":1}}");
    }

    public function testEncodeTopLevelAny()
    {
        // Test a normal message.
        $packed = new TestMessage();
        $packed->setOptionalInt32(123);
        $packed->setOptionalString("abc");

        $m = new Any();
        $m->pack($packed);
        $expected1 =
            "{\"@type\":\"type.googleapis.com/foo.TestMessage\"," .
            "\"optional_int32\":123,\"optional_string\":\"abc\"}";
        $expected2 =
            "{\"@type\":\"type.googleapis.com/foo.TestMessage\"," .
            "\"optionalInt32\":123,\"optionalString\":\"abc\"}";
        $result = $m->serializeToJsonString();
        $this->assertTrue($expected1 === $result || $expected2 === $result);

        // Test a well known message.
        $packed = new Int32Value();
        $packed->setValue(123);

        $m = new Any();
        $m->pack($packed);
        $this->assertSame(
            "{\"@type\":\"type.googleapis.com/google.protobuf.Int32Value\"," .
            "\"value\":123}",
            $m->serializeToJsonString());

        // Test an Any message.
        $outer = new Any();
        $outer->pack($m);
        $this->assertSame(
            "{\"@type\":\"type.googleapis.com/google.protobuf.Any\"," .
            "\"value\":{\"@type\":\"type.googleapis.com/google.protobuf.Int32Value\"," .
            "\"value\":123}}",
            $outer->serializeToJsonString());

        // Test a Timestamp message.
        $packed = new Google\Protobuf\Timestamp();
        $packed->setSeconds(946684800);
        $packed->setNanos(123456789);
        $m = new Any();
        $m->pack($packed);
        $this->assertSame(
            "{\"@type\":\"type.googleapis.com/google.protobuf.Timestamp\"," .
            "\"value\":\"2000-01-01T00:00:00.123456789Z\"}",
            $m->serializeToJsonString());
    }

    public function testEncodeAnyWithDefaultWrapperMessagePacked()
    {
        $any = new Any();
        $any->pack(new TestInt32Value([
            'field' => new Int32Value(['value' => 0]),
        ]));
        $this->assertSame(
            "{\"@type\":\"type.googleapis.com/foo.TestInt32Value\"," .
            "\"field\":0}",
            $any->serializeToJsonString());
    }

    public function testDecodeTopLevelFieldMask()
    {
        $m = new TestMessage();
        $m->setMapStringString(['a'=>'abcdefg']);
        $data1 = $m->serializeToJsonString();
        $n = new TestMessage();
        $n->mergeFromJsonString($data1);
        $data2 = $n->serializeToJsonString();
        $this->assertSame($data1, $data2);

        $m = new FieldMask();
        $m->mergeFromJsonString("\"foo.barBaz,qux\"");
        $this->assertSame("foo.bar_baz", $m->getPaths()[0]);
        $this->assertSame("qux", $m->getPaths()[1]);
    }

    public function testEncodeTopLevelFieldMask()
    {
        $m = new FieldMask();
        $m->setPaths(["foo.bar_baz", "qux"]);
        $this->assertSame("\"foo.barBaz,qux\"", $m->serializeToJsonString());
    }

    public function testDecodeEmptyFieldMask()
    {
        $m = new FieldMask();
        $m->mergeFromJsonString("\"\"");
        $this->assertEquals("", $m->serializeToString());
    }

    public function testJsonDecodeMapWithDefaultValueKey()
    {
        $m = new TestMessage();
        $m->getMapInt32Int32()[0] = 0;
        $this->assertSame("{\"mapInt32Int32\":{\"0\":0}}",
                          $m->serializeToJsonString());

        $m = new TestMessage();
        $m->getMapStringString()[""] = "";
        $this->assertSame("{\"mapStringString\":{\"\":\"\"}}",
                          $m->serializeToJsonString());
    }

    public function testJsonDecodeNumericStringMapKey()
    {
        $m = new TestMessage();
        $m->getMapStringString()["1"] = "1";
        $data = $m->serializeToJsonString();
        $this->assertSame("{\"mapStringString\":{\"1\":\"1\"}}", $data);
        $n = new TestMessage();
        $n->mergeFromJsonString($data);
    }

    public function testMessageMapNoValue()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin("CA0700"));
        $m->serializeToString();
        $this->assertTrue(true);
    }

    public function testAnyMapNoValue()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin("D20700"));
        $m->serializeToString();
        $this->assertTrue(true);
    }

    public function testListValueMapNoValue()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin("DA0700"));
        $m->serializeToString();
        $this->assertTrue(true);
    }

    public function testStructMapNoValue()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin("E20700"));
        $m->serializeToString();
        $this->assertTrue(true);
    }

    /**
     * @dataProvider wrappersDataProvider
     */
    public function testWrapperJsonDecodeAndGet(
        $class,
        $nonDefaultValue,
        $nonDefaultValueData,
        $defaultValue,
        $defaultValueData
    )
    {
        // Singular with non-default
        $m = new $class();
        $m->mergeFromJsonString("{\"field\":" . $nonDefaultValueData . "}");
        $wrapper = $m->getField();
        $this->assertEquals($nonDefaultValue, $wrapper->getValue());

        // Singular with default
        $m = new $class();
        $m->mergeFromJsonString("{\"field\":" . $defaultValueData . "}");
        $wrapper = $m->getField();
        $this->assertEquals($defaultValue, $wrapper->getValue());

        // Repeated with empty
        $m = new $class();
        $m->mergeFromJsonString("{\"repeated_field\":[]}");
        $repeatedWrapper = $m->getRepeatedField();
        $this->assertSame(0, count($repeatedWrapper));

        // Repeated with non-default
        $m = new $class();
        $m->mergeFromJsonString("{\"repeated_field\":[" . $defaultValueData . "]}");
        $repeatedWrapper = $m->getRepeatedField();
        $this->assertSame(1, count($repeatedWrapper));
        $this->assertEquals($defaultValue, $repeatedWrapper[0]->getValue());

        // Repeated with default
        $m = new $class();
        $m->mergeFromJsonString("{\"repeated_field\":[" . $defaultValueData . "]}");
        $repeatedWrapper = $m->getRepeatedField();
        $this->assertSame(1, count($repeatedWrapper));
        $this->assertEquals($defaultValue, $repeatedWrapper[0]->getValue());

        // Oneof with non-default
        $m = new $class();
        $m->mergeFromJsonString("{\"oneof_field\":" . $nonDefaultValueData . "}");
        $wrapper = $m->getOneofField();
        $this->assertEquals($nonDefaultValue, $wrapper->getValue());
        $this->assertEquals("oneof_field", $m->getOneofFields());
        $this->assertEquals(0, $m->getInt32Field());

        // Oneof with default
        $m = new $class();
        $m->mergeFromJsonString("{\"oneof_field\":" . $defaultValueData . "}");
        $wrapper = $m->getOneofField();
        $this->assertEquals($defaultValue, $wrapper->getValue());
        $this->assertEquals("oneof_field", $m->getOneofFields());
        $this->assertEquals(0, $m->getInt32Field());
    }

    /**
     * @dataProvider wrappersDataProvider
     */
    public function testWrapperJsonDecodeAndGetUnwrapped(
        $class,
        $nonDefaultValue,
        $nonDefaultValueData,
        $defaultValue,
        $defaultValueData
    )
    {
        // Singular with non-default
        $m = new $class();
        $m->mergeFromJsonString("{\"field\":" . $nonDefaultValueData . "}");
        $this->assertEquals($nonDefaultValue, $m->getFieldUnwrapped());

        // Singular with default
        $m = new $class();
        $m->mergeFromJsonString("{\"field\":" . $defaultValueData . "}");
        $this->assertEquals($defaultValue, $m->getFieldUnwrapped());

        // Oneof with non-default
        $m = new $class();
        $m->mergeFromJsonString("{\"oneof_field\":" . $nonDefaultValueData . "}");
        $this->assertEquals($nonDefaultValue, $m->getOneofFieldUnwrapped());
        $this->assertEquals("oneof_field", $m->getOneofFields());
        $this->assertEquals(0, $m->getInt32Field());

        // Oneof with default
        $m = new $class();
        $m->mergeFromJsonString("{\"oneof_field\":" . $defaultValueData . "}");
        $this->assertEquals($defaultValue, $m->getOneofFieldUnwrapped());
        $this->assertEquals("oneof_field", $m->getOneofFields());
        $this->assertEquals(0, $m->getInt32Field());
    }

    /**
     * @dataProvider wrappersDataProvider
     */
    public function testWrapperJsonDecodeEncode(
        $class,
        $nonDefaultValue,
        $nonDefaultValueData,
        $defaultValue,
        $defaultValueData
    )
    {
        // Singular with non-default
        $from = new $class();
        $to = new $class();
        $from->mergeFromJsonString("{\"field\":" . $nonDefaultValueData . "}");
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($nonDefaultValue, $to->getFieldUnwrapped());

        // Singular with default
        $from = new $class();
        $to = new $class();
        $from->mergeFromJsonString("{\"field\":" . $defaultValueData . "}");
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($defaultValue, $to->getFieldUnwrapped());

        // Oneof with non-default
        $from = new $class();
        $to = new $class();
        $from->mergeFromJsonString("{\"oneof_field\":" . $nonDefaultValueData . "}");
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($nonDefaultValue, $to->getOneofFieldUnwrapped());

        // Oneof with default
        $from = new $class();
        $to = new $class();
        $from->mergeFromJsonString("{\"oneof_field\":" . $defaultValueData . "}");
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($defaultValue, $to->getOneofFieldUnwrapped());
    }

    /**
     * @dataProvider wrappersDataProvider
     */
    public function testWrapperSetUnwrappedJsonEncode(
        $class,
        $nonDefaultValue,
        $nonDefaultValueData,
        $defaultValue,
        $defaultValueData
    )
    {
        // Singular with non-default
        $from = new $class();
        $to = new $class();
        $from->setFieldUnwrapped($nonDefaultValue);
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($nonDefaultValue, $to->getFieldUnwrapped());

        // Singular with default
        $from = new $class();
        $to = new $class();
        $from->setFieldUnwrapped($defaultValue);
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($defaultValue, $to->getFieldUnwrapped());

        // Oneof with non-default
        $from = new $class();
        $to = new $class();
        $from->setOneofFieldUnwrapped($nonDefaultValue);
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($nonDefaultValue, $to->getOneofFieldUnwrapped());

        // Oneof with default
        $from = new $class();
        $to = new $class();
        $from->setOneofFieldUnwrapped($defaultValue);
        $data = $from->serializeToJsonString();
        $to->mergeFromJsonString($data);
        $this->assertEquals($defaultValue, $to->getOneofFieldUnwrapped());
    }

    public function wrappersDataProvider()
    {
        return [
            [TestInt32Value::class, 1, "1", 0, "0"],
            [TestStringValue::class, "a", "\"a\"", "", "\"\""],
        ];
    }
}
