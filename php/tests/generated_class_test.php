<?php

require_once('generated/NoNamespaceEnum.php');
require_once('generated/NoNamespaceMessage.php');
require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestEnum;
use Foo\TestIncludePrefixMessage;
use Foo\TestMessage;
use Foo\TestMessage_Sub;

class GeneratedClassTest extends TestBase
{

    #########################################################
    # Test field accessors.
    #########################################################

    public function testSetterGetter()
    {
        $m = new TestMessage();
        $m->setOptionalInt32(1);
        $this->assertSame(1, $m->getOptionalInt32());
    }

    #########################################################
    # Test int32 field.
    #########################################################

    public function testInt32Field()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalInt32(MAX_INT32);
        $this->assertSame(MAX_INT32, $m->getOptionalInt32());
        $m->setOptionalInt32(MIN_INT32);
        $this->assertSame(MIN_INT32, $m->getOptionalInt32());

        // Set float.
        $m->setOptionalInt32(1.1);
        $this->assertSame(1, $m->getOptionalInt32());
        $m->setOptionalInt32(MAX_INT32_FLOAT);
        $this->assertSame(MAX_INT32, $m->getOptionalInt32());
        $m->setOptionalInt32(MIN_INT32_FLOAT);
        $this->assertSame(MIN_INT32, $m->getOptionalInt32());

        // Set string.
        $m->setOptionalInt32('2');
        $this->assertSame(2, $m->getOptionalInt32());
        $m->setOptionalInt32('3.1');
        $this->assertSame(3, $m->getOptionalInt32());
        $m->setOptionalInt32(MAX_INT32_STRING);
        $this->assertSame(MAX_INT32, $m->getOptionalInt32());
        $m->setOptionalInt32(MIN_INT32_STRING);
        $this->assertSame(MIN_INT32, $m->getOptionalInt32());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32FieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalInt32(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32FieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalInt32('abc');
    }

    #########################################################
    # Test uint32 field.
    #########################################################

    public function testUint32Field()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalUint32(MAX_UINT32);
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32(-1);
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32(MIN_UINT32);
        $this->assertSame(MIN_INT32, $m->getOptionalUint32());

        // Set float.
        $m->setOptionalUint32(1.1);
        $this->assertSame(1, $m->getOptionalUint32());
        $m->setOptionalUint32(MAX_UINT32_FLOAT);
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32(-1.0);
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32(MIN_UINT32_FLOAT);
        $this->assertSame(MIN_INT32, $m->getOptionalUint32());

        // Set string.
        $m->setOptionalUint32('2');
        $this->assertSame(2, $m->getOptionalUint32());
        $m->setOptionalUint32('3.1');
        $this->assertSame(3, $m->getOptionalUint32());
        $m->setOptionalUint32(MAX_UINT32_STRING);
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32('-1.0');
        $this->assertSame(-1, $m->getOptionalUint32());
        $m->setOptionalUint32(MIN_UINT32_STRING);
        $this->assertSame(MIN_INT32, $m->getOptionalUint32());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32FieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalUint32(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32FieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalUint32('abc');
    }

    #########################################################
    # Test int64 field.
    #########################################################

    public function testInt64Field()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalInt64(MAX_INT64);
        $this->assertSame(MAX_INT64, $m->getOptionalInt64());
        $m->setOptionalInt64(MIN_INT64);
        $this->assertEquals(MIN_INT64, $m->getOptionalInt64());

        // Set float.
        $m->setOptionalInt64(1.1);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('1', $m->getOptionalInt64());
        } else {
            $this->assertSame(1, $m->getOptionalInt64());
        }

        // Set string.
        $m->setOptionalInt64('2');
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('2', $m->getOptionalInt64());
        } else {
            $this->assertSame(2, $m->getOptionalInt64());
        }

        $m->setOptionalInt64('3.1');
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('3', $m->getOptionalInt64());
        } else {
            $this->assertSame(3, $m->getOptionalInt64());
        }

        $m->setOptionalInt64(MAX_INT64_STRING);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_INT64_STRING, $m->getOptionalInt64());
        } else {
            $this->assertSame(MAX_INT64, $m->getOptionalInt64());
        }

        $m->setOptionalInt64(MIN_INT64_STRING);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MIN_INT64_STRING, $m->getOptionalInt64());
        } else {
            $this->assertSame(MIN_INT64, $m->getOptionalInt64());
        }
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64FieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalInt64(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64FieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalInt64('abc');
    }

    #########################################################
    # Test uint64 field.
    #########################################################

    public function testUint64Field()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalUint64(MAX_UINT64);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_UINT64_STRING, $m->getOptionalUint64());
        } else {
            $this->assertSame(MAX_UINT64, $m->getOptionalUint64());
        }

        // Set float.
        $m->setOptionalUint64(1.1);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('1', $m->getOptionalUint64());
        } else {
            $this->assertSame(1, $m->getOptionalUint64());
        }

        // Set string.
        $m->setOptionalUint64('2');
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('2', $m->getOptionalUint64());
        } else {
            $this->assertSame(2, $m->getOptionalUint64());
        }

        $m->setOptionalUint64('3.1');
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('3', $m->getOptionalUint64());
        } else {
            $this->assertSame(3, $m->getOptionalUint64());
        }

        $m->setOptionalUint64(MAX_UINT64_STRING);
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_UINT64_STRING, $m->getOptionalUint64());
        } else {
            $this->assertSame(MAX_UINT64, $m->getOptionalUint64());
        }
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64FieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalUint64(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64FieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalUint64('abc');
    }

    #########################################################
    # Test enum field.
    #########################################################

    public function testEnumField()
    {
        $m = new TestMessage();

        // Set enum.
        $m->setOptionalEnum(TestEnum::ONE);
        $this->assertEquals(TestEnum::ONE, $m->getOptionalEnum());

        // Set integer.
        $m->setOptionalEnum(1);
        $this->assertEquals(TestEnum::ONE, $m->getOptionalEnum());

        // Set float.
        $m->setOptionalEnum(1.1);
        $this->assertEquals(TestEnum::ONE, $m->getOptionalEnum());

        // Set string.
        $m->setOptionalEnum("1");
        $this->assertEquals(TestEnum::ONE, $m->getOptionalEnum());
    }

    public function testNestedEnum()
    {
        $m = new TestMessage();
        $m->setOptionalNestedEnum(\Foo\TestMessage_NestedEnum::ZERO);
    }

    #########################################################
    # Test float field.
    #########################################################

    public function testFloatField()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalFloat(1);
        $this->assertEquals(1.0, $m->getOptionalFloat(), '', MAX_FLOAT_DIFF);

        // Set float.
        $m->setOptionalFloat(1.1);
        $this->assertEquals(1.1, $m->getOptionalFloat(), '', MAX_FLOAT_DIFF);

        // Set string.
        $m->setOptionalFloat('2');
        $this->assertEquals(2.0, $m->getOptionalFloat(), '', MAX_FLOAT_DIFF);
        $m->setOptionalFloat('3.1');
        $this->assertEquals(3.1, $m->getOptionalFloat(), '', MAX_FLOAT_DIFF);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatFieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalFloat(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatFieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalFloat('abc');
    }

    #########################################################
    # Test double field.
    #########################################################

    public function testDoubleField()
    {
        $m = new TestMessage();

        // Set integer.
        $m->setOptionalDouble(1);
        $this->assertEquals(1.0, $m->getOptionalDouble(), '', MAX_FLOAT_DIFF);

        // Set float.
        $m->setOptionalDouble(1.1);
        $this->assertEquals(1.1, $m->getOptionalDouble(), '', MAX_FLOAT_DIFF);

        // Set string.
        $m->setOptionalDouble('2');
        $this->assertEquals(2.0, $m->getOptionalDouble(), '', MAX_FLOAT_DIFF);
        $m->setOptionalDouble('3.1');
        $this->assertEquals(3.1, $m->getOptionalDouble(), '', MAX_FLOAT_DIFF);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleFieldInvalidTypeFail()
    {
        $m = new TestMessage();
        $m->setOptionalDouble(new TestMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleFieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalDouble('abc');
    }

    #########################################################
    # Test bool field.
    #########################################################

    public function testBoolField()
    {
        $m = new TestMessage();

        // Set bool.
        $m->setOptionalBool(true);
        $this->assertSame(true, $m->getOptionalBool());

        // Set integer.
        $m->setOptionalBool(-1);
        $this->assertSame(true, $m->getOptionalBool());

        // Set float.
        $m->setOptionalBool(1.1);
        $this->assertSame(true, $m->getOptionalBool());

        // Set string.
        $m->setOptionalBool('');
        $this->assertSame(false, $m->getOptionalBool());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolFieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalBool(new TestMessage());
    }

    #########################################################
    # Test string field.
    #########################################################

    public function testStringField()
    {
        $m = new TestMessage();

        // Set string.
        $m->setOptionalString('abc');
        $this->assertSame('abc', $m->getOptionalString());

        // Set integer.
        $m->setOptionalString(1);
        $this->assertSame('1', $m->getOptionalString());

        // Set double.
        $m->setOptionalString(1.1);
        $this->assertSame('1.1', $m->getOptionalString());

        // Set bool.
        $m->setOptionalString(true);
        $this->assertSame('1', $m->getOptionalString());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringFieldInvalidUTF8Fail()
    {
        $m = new TestMessage();
        $hex = hex2bin("ff");
        $m->setOptionalString($hex);
    }

    #########################################################
    # Test bytes field.
    #########################################################

    public function testBytesField()
    {
        $m = new TestMessage();

        // Set string.
        $m->setOptionalBytes('abc');
        $this->assertSame('abc', $m->getOptionalBytes());

        // Set integer.
        $m->setOptionalBytes(1);
        $this->assertSame('1', $m->getOptionalBytes());

        // Set double.
        $m->setOptionalBytes(1.1);
        $this->assertSame('1.1', $m->getOptionalBytes());

        // Set bool.
        $m->setOptionalBytes(true);
        $this->assertSame('1', $m->getOptionalBytes());
    }

      public function testBytesFieldInvalidUTF8Success()
      {
          $m = new TestMessage();
          $hex = hex2bin("ff");
          $m->setOptionalBytes($hex);
      }

    #########################################################
    # Test message field.
    #########################################################

    public function testMessageField()
    {
        $m = new TestMessage();

        $sub_m = new TestMessage_Sub();
        $sub_m->setA(1);
        $m->setOptionalMessage($sub_m);
        $this->assertSame(1, $m->getOptionalMessage()->getA());

        $null = null;
        $m->setOptionalMessage($null);
        $this->assertNull($m->getOptionalMessage());
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageFieldWrongTypeFail()
    {
        $m = new TestMessage();
        $a = 1;
        $m->setOptionalMessage($a);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageFieldWrongClassFail()
    {
        $m = new TestMessage();
        $m->setOptionalMessage(new TestMessage());
    }

    #########################################################
    # Test repeated field.
    #########################################################

    public function testRepeatedField()
    {
        $m = new TestMessage();

        $repeated_int32 = new RepeatedField(GPBType::INT32);
        $m->setRepeatedInt32($repeated_int32);
        $this->assertSame($repeated_int32, $m->getRepeatedInt32());
    }

    public function testRepeatedFieldViaArray()
    {
        $m = new TestMessage();

        $arr = array();
        $m->setRepeatedInt32($arr);
        $this->assertSame(0, count($m->getRepeatedInt32()));

        $arr = array(1, 2.1, "3");
        $m->setRepeatedInt32($arr);
        $this->assertTrue($m->getRepeatedInt32() instanceof RepeatedField);
        $this->assertSame("Google\Protobuf\Internal\RepeatedField",
                          get_class($m->getRepeatedInt32()));
        $this->assertSame(3, count($m->getRepeatedInt32()));
        $this->assertSame(1, $m->getRepeatedInt32()[0]);
        $this->assertSame(2, $m->getRepeatedInt32()[1]);
        $this->assertSame(3, $m->getRepeatedInt32()[2]);
        $this->assertFalse($arr instanceof RepeatedField);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRepeatedFieldWrongTypeFail()
    {
        $m = new TestMessage();
        $a = 1;
        $m->setRepeatedInt32($a);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRepeatedFieldWrongObjectFail()
    {
        $m = new TestMessage();
        $m->setRepeatedInt32($m);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRepeatedFieldWrongRepeatedTypeFail()
    {
        $m = new TestMessage();

        $repeated_int32 = new RepeatedField(GPBType::UINT32);
        $m->setRepeatedInt32($repeated_int32);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRepeatedFieldWrongRepeatedMessageClassFail()
    {
        $m = new TestMessage();

        $repeated_message = new RepeatedField(GPBType::MESSAGE,
                                              TestMessage::class);
        $m->setRepeatedMessage($repeated_message);
    }

    #########################################################
    # Test map field.
    #########################################################

    public function testMapField()
    {
        $m = new TestMessage();

        $map_int32_int32 = new MapField(GPBType::INT32, GPBType::INT32);
        $m->setMapInt32Int32($map_int32_int32);
        $this->assertSame($map_int32_int32, $m->getMapInt32Int32());
    }

    public function testMapFieldViaArray()
    {
        $m = new TestMessage();

        $dict = array();
        $m->setMapInt32Int32($dict);
        $this->assertSame(0, count($m->getMapInt32Int32()));

        $dict = array(5 => 5, 6.1 => 6.1, "7" => "7");
        $m->setMapInt32Int32($dict);
        $this->assertTrue($m->getMapInt32Int32() instanceof MapField);
        $this->assertSame(3, count($m->getMapInt32Int32()));
        $this->assertSame(5, $m->getMapInt32Int32()[5]);
        $this->assertSame(6, $m->getMapInt32Int32()[6]);
        $this->assertSame(7, $m->getMapInt32Int32()[7]);
        $this->assertFalse($dict instanceof MapField);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMapFieldWrongTypeFail()
    {
        $m = new TestMessage();
        $a = 1;
        $m->setMapInt32Int32($a);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMapFieldWrongObjectFail()
    {
        $m = new TestMessage();
        $m->setMapInt32Int32($m);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMapFieldWrongRepeatedTypeFail()
    {
        $m = new TestMessage();

        $map_uint32_uint32 = new MapField(GPBType::UINT32, GPBType::UINT32);
        $m->setMapInt32Int32($map_uint32_uint32);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMapFieldWrongRepeatedMessageClassFail()
    {
        $m = new TestMessage();

        $map_int32_message = new MapField(GPBType::INT32,
                                          GPBType::MESSAGE,
                                          TestMessage::class);
        $m->setMapInt32Message($map_int32_message);
    }

    #########################################################
    # Test oneof field.
    #########################################################

    public function testOneofField() {
        $m = new TestMessage();

        $this->assertSame("", $m->getMyOneof());

        $m->setOneofInt32(1);
        $this->assertSame(1, $m->getOneofInt32());
        $this->assertSame(0.0, $m->getOneofFloat());
        $this->assertSame('', $m->getOneofString());
        $this->assertSame(NULL, $m->getOneofMessage());
        $this->assertSame("oneof_int32", $m->getMyOneof());

        $m->setOneofFloat(2.0);
        $this->assertSame(0, $m->getOneofInt32());
        $this->assertSame(2.0, $m->getOneofFloat());
        $this->assertSame('', $m->getOneofString());
        $this->assertSame(NULL, $m->getOneofMessage());
        $this->assertSame("oneof_float", $m->getMyOneof());

        $m->setOneofString('abc');
        $this->assertSame(0, $m->getOneofInt32());
        $this->assertSame(0.0, $m->getOneofFloat());
        $this->assertSame('abc', $m->getOneofString());
        $this->assertSame(NULL, $m->getOneofMessage());
        $this->assertSame("oneof_string", $m->getMyOneof());

        $sub_m = new TestMessage_Sub();
        $sub_m->setA(1);
        $m->setOneofMessage($sub_m);
        $this->assertSame(0, $m->getOneofInt32());
        $this->assertSame(0.0, $m->getOneofFloat());
        $this->assertSame('', $m->getOneofString());
        $this->assertSame(1, $m->getOneofMessage()->getA());
        $this->assertSame("oneof_message", $m->getMyOneof());
    }

    #########################################################
    # Test clear method.
    #########################################################

    public function testMessageClear()
    {
        $m = new TestMessage();
        $this->setFields($m);
        $this->expectFields($m);
        $m->clear();
        $this->expectEmptyFields($m);
    }

    #########################################################
    # Test mergeFrom method.
    #########################################################

    public function testMessageMergeFrom()
    {
        $m = new TestMessage();
        $this->setFields($m);
        $this->expectFields($m);
        $arr = $m->getOptionalMessage()->getB();
        $arr[] = 1;

        $n = new TestMessage();

        // Singular
        $n->setOptionalInt32(100);
        $sub1 = new TestMessage_Sub();
        $sub1->setA(101);
        $sub1->getB()[] = 102;
        $n->setOptionalMessage($sub1);

        // Repeated
        $n->getRepeatedInt32()[] = 200;
        $n->getRepeatedString()[] = 'abc';
        $sub2 = new TestMessage_Sub();
        $sub2->setA(201);
        $n->getRepeatedMessage()[] = $sub2;

        // Map
        $n->getMapInt32Int32()[1] = 300;
        $n->getMapInt32Int32()[-62] = 301;
        $n->getMapStringString()['def'] = 'def';
        $n->getMapInt32Message()[1] = new TestMessage_Sub();
        $n->getMapInt32Message()[1]->setA(302);
        $n->getMapInt32Message()[2] = new TestMessage_Sub();
        $n->getMapInt32Message()[2]->setA(303);

        $m->mergeFrom($n);

        $this->assertSame(100, $m->getOptionalInt32());
        $this->assertSame(42, $m->getOptionalUint32());
        $this->assertSame(101, $m->getOptionalMessage()->getA());
        $this->assertSame(2, count($m->getOptionalMessage()->getB()));
        $this->assertSame(1, $m->getOptionalMessage()->getB()[0]);
        $this->assertSame(102, $m->getOptionalMessage()->getB()[1]);

        $this->assertSame(3, count($m->getRepeatedInt32()));
        $this->assertSame(200, $m->getRepeatedInt32()[2]);
        $this->assertSame(2, count($m->getRepeatedUint32()));
        $this->assertSame(3, count($m->getRepeatedString()));
        $this->assertSame('abc', $m->getRepeatedString()[2]);
        $this->assertSame(3, count($m->getRepeatedMessage()));
        $this->assertSame(201, $m->getRepeatedMessage()[2]->getA());

        $this->assertSame(2, count($m->getMapInt32Int32()));
        $this->assertSame(300, $m->getMapInt32Int32()[1]);
        $this->assertSame(301, $m->getMapInt32Int32()[-62]);
        $this->assertSame(1, count($m->getMapUint32Uint32()));
        $this->assertSame(2, count($m->getMapStringString()));
        $this->assertSame('def', $m->getMapStringString()['def']);

        $this->assertSame(2, count($m->getMapInt32Message()));
        $this->assertSame(302, $m->getMapInt32Message()[1]->getA());
        $this->assertSame(303, $m->getMapInt32Message()[2]->getA());

        $this->assertSame("", $m->getMyOneof());

        // Check sub-messages are copied by value.
        $n->getOptionalMessage()->setA(-101);
        $this->assertSame(101, $m->getOptionalMessage()->getA());
        $n->getRepeatedMessage()[0]->setA(-201);
        $this->assertSame(201, $m->getRepeatedMessage()[2]->getA());
        $n->getMapInt32Message()[1]->setA(-302);
        $this->assertSame(302, $m->getMapInt32Message()[1]->getA());

        // Test merge oneof.
        $m = new TestMessage();

        $n = new TestMessage();
        $n->setOneofInt32(1);
        $m->mergeFrom($n);
        $this->assertSame(1, $m->getOneofInt32());

        $sub = new TestMessage_Sub();
        $n->setOneofMessage($sub);
        $n->getOneofMessage()->setA(400);
        $m->mergeFrom($n);
        $this->assertSame(400, $m->getOneofMessage()->getA());
        $n->getOneofMessage()->setA(-400);
        $this->assertSame(400, $m->getOneofMessage()->getA());

        // Test all fields
        $m = new TestMessage();
        $n = new TestMessage();
        $this->setFields($m);
        $n->mergeFrom($m);
        $this->expectFields($n);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageMergeFromInvalidTypeFail()
    {
        $m = new TestMessage();
        $n = new TestMessage_Sub();
        $m->mergeFrom($n);
    }

    #########################################################
    # Test message/enum without namespace.
    #########################################################

    public function testMessageWithoutNamespace()
    {
        $m = new TestMessage();
        $sub = new NoNameSpaceMessage();
        $m->setOptionalNoNamespaceMessage($sub);
        $m->getRepeatedNoNamespaceMessage()[] = new NoNameSpaceMessage();

        $n = new NoNamespaceMessage();
        $n->setB(NoNamespaceMessage_NestedEnum::ZERO);
    }

    public function testEnumWithoutNamespace()
    {
        $m = new TestMessage();
        $m->setOptionalNoNamespaceEnum(NoNameSpaceEnum::VALUE_A);
        $m->getRepeatedNoNamespaceEnum()[] = NoNameSpaceEnum::VALUE_A;
    }

    #########################################################
    # Test message with given prefix.
    #########################################################

    public function testPrefixMessage()
    {
        $m = new TestIncludePrefixMessage();
        $n = new PrefixTestPrefix();
        $n->setA(1);
        $m->setPrefixMessage($n);
        $this->assertSame(1, $m->getPrefixMessage()->getA());
    }

    #########################################################
    # Test prefix for reserved words.
    #########################################################

    public function testPrefixForReservedWords()
    {
        $m = new \Foo\TestMessage_Empty();
        $m = new \Foo\PBEmpty();
        $m = new \PrefixEmpty();
    }
}
