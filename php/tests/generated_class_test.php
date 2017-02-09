<?php

require_once('generated/NoNameSpaceEnum.php');
require_once('generated/NoNameSpaceMessage.php');
require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestEnum;
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
    # Test message/enum without namespace.
    #########################################################

    public function testMessageWithoutNamespace()
    {
        $m = new NoNameSpaceMessage();
    }

    public function testEnumWithoutNamespace()
    {
        $m = new NoNameSpaceEnum();
    }
}
