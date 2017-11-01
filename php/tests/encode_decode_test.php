<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Any;
use Google\Protobuf\Duration;
use Google\Protobuf\Int32Value;
use Google\Protobuf\BoolValue;
use Google\Protobuf\StringValue;
use Google\Protobuf\BytesValue;
use Google\Protobuf\Value;
use Google\Protobuf\ListValue;
use Google\Protobuf\Struct;
use Google\Protobuf\NullValue;
use Google\Protobuf\FieldMask;
use Foo\AnyWrapper;

use Google\Protobuf\RepeatedField;
use Google\Protobuf\GPBType;
use Foo\TestEnum;
use Foo\TestMessage;
use Foo\TestMessage_Sub;
use Foo\TestPackedMessage;
use Foo\TestRandomFieldOrder;
use Foo\TestUnpackedMessage;

class EncodeDecodeTest extends TestBase
{

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

        $sub_m = new TestMessage_Sub();
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

        $sub_m = new TestMessage_Sub();
        $m->setOneofMessage($sub_m);
        $data = $m->serializeToString();
        $n = new TestMessage();
        $n->mergeFromString($data);
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
    }

    public function testPackedDecodeUnpacked()
    {
        $to = new TestPackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
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
    }

    public function testUnpackedDecodeUnpacked()
    {
        $to = new TestUnpackedMessage();
        $to->mergeFromString(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
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

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidInt32()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('08'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidSubMessage()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('9A010108'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidInt64()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('10'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidUInt32()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('18'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidUInt64()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('20'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidSInt32()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('28'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidSInt64()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('30'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidFixed32()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('3D'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidFixed64()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('41'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidSFixed32()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('4D'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidSFixed64()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('51'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidFloat()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('5D'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidDouble()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('61'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidBool()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('68'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidStringLengthMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('72'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidStringDataMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7201'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidBytesLengthMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7A'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidBytesDataMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('7A01'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidEnum()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8001'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidMessageLengthMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8A01'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidMessageDataMiss()
    {
        $m = new TestMessage();
        $m->mergeFromString(hex2bin('8A0101'));
    }

    /**
     * @expectedException Exception
     */
    public function testDecodeInvalidPackedMessageLength()
    {
        $m = new TestPackedMessage();
        $m->mergeFromString(hex2bin('D205'));
    }

    public function testUnknown()
    {
        $m = new TestMessage();
        $from = hex2bin('F80601');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        $m = new TestMessage();
        $from = hex2bin('F9060000000000000000');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        $m = new TestMessage();
        $from = hex2bin('FA0600');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));

        $m = new TestMessage();
        $from = hex2bin('FD0600000000');
        $m->mergeFromString($from);
        $to = $m->serializeToString();
        $this->assertSame(bin2hex($from), bin2hex($to));
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

    public function testAnyJsonEncode()
    {
        $from = new AnyWrapper();
#         ## Any
#         $any = new Any();
# 
# #         $nested = new Any();
# #         $value_msg = new TestMessage();
# #         $value_msg->setOptionalInt32(12345);
# #         $any_nested->pack($value_msg);
# 
# #         $value_msg = new Duration();
# #         $value_msg->setSeconds(1);
# #         $value_msg->setNanos(2);
# 
# #         $value_msg = new Int32Value();
# #         $value_msg->setValue(1);
# 
# #         $value_msg = new Value();
# #         $value_msg->setNumberValue(1);
# 
# #         $value_msg = new FieldMask();
# #         $paths = $value_msg->getPaths();
# #         $paths []= "user.display_name";
# #         $paths []= "photo";
# 
#         $any->pack($value_msg);
#         $from->setAny($any);

#         ## Int32Value
#         $int32_wrapper = new Int32Value();
#         $int32_wrapper->setValue(1);
#         $from->setInt32Wrapper($int32_wrapper);

#         ## BoolValue
#         $bool_wrapper = new BoolValue();
#         $bool_wrapper->setValue(true);
#         $from->setBoolWrapper($bool_wrapper);

#         ## StringValue
#         $string_wrapper = new StringValue();
#         $string_wrapper->setValue("abc");
#         $from->setStringWrapper($string_wrapper);

#         ## BytesValue
#         $bytes_wrapper = new BytesValue();
#         $bytes_wrapper->setValue("abc");
#         $from->setBytesWrapper($bytes_wrapper);

#         ## Value
#         $value = new Value();
# 
# #         $value->setBoolValue(true);
# 
# #         $v1 = new Value();
# #         $v1->setNumberValue(1);
# #         $v2 = new Value();
# #         $v2->setNumberValue(2);
# #         $list = new ListValue();
# #         $list->setValues([$v1, $v2]);
# #         $value->setListValue($list);
# 
# #         $v1 = new Value();
# #         $v1->setNumberValue(1);
# #         $v2 = new Value();
# #         $v2->setNumberValue(2);
# #         $struct = new Struct();
# #         $struct->setFields(["a"=>$v1,"b"=>$v2]);
# #         $value->setStructValue($struct);
# 
#         $value->setNullValue(NullValue::NULL_VALUE);
# 
#         $from->setValue($value);

        ## BytesValue
        var_dump("SET UP VALUE");

#         $bytes_wrapper = new BytesValue();
#         $bytes_wrapper->setValue(hex2bin("0102"));
#         $from->setBytesWrapper($bytes_wrapper);
#         $from->setBytesWrapper($bytes_wrapper);

        $repeated_bytes_wrapper = $from->getRepeatedBytesWrapper();
        $bytes_wrapper = new BytesValue();
        $bytes_wrapper->setValue(hex2bin(""));
        $repeated_bytes_wrapper[] = $bytes_wrapper;
        $bytes_wrapper = new BytesValue();
        $bytes_wrapper->setValue(hex2bin("0102"));
        $repeated_bytes_wrapper[] = $bytes_wrapper;

#         ## Struct
#         $struct = new Struct();
#         $map = $struct->getFields();
#         $value = new Value();
#         $value->setBoolValue(true);
#         $map["a"] = $value;
#         $from->setStruct($struct);

#         ## Duration
#         $duration = new Duration();
#         $duration->setSeconds(-315576000001);
#         $duration->setNanos(0);
#         $from->setDuration($duration);

#         ## FieldMask
#         $field_mask = new FieldMask();
#         $paths = $field_mask->getPaths();
#         $paths []= "user.display_name";
#         $paths []= "photo";
#         $from->setFieldMask($field_mask);

        var_dump ("TEST START");
 #        $data = $from->serializeToJsonString();
        $data = "{\"repeatedBytesWrapper\":[\"\", \"AQI=\"]}";
        var_dump($data);

        $to = new AnyWrapper();
        $to->mergeFromJsonString($data);
        var_dump($to->serializeToJsonString());
    }
}
