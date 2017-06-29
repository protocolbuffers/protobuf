<?php

require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestMessage;
use Foo\TestMessage_Sub;

class UndefinedTest extends PHPUnit_Framework_TestCase
{

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32AppendStringFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetStringFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32AppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetMessageFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32AppendStringFail()
    {
        $arr = new RepeatedField(GPBType::UINT32);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetStringFail()
    {
        $arr = new RepeatedField(GPBType::UINT32);
        $arr[] = 0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32AppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::UINT32);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetMessageFail()
    {
        $arr = new RepeatedField(GPBType::UINT32);
        $arr[] = 0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64AppendStringFail()
    {
        $arr = new RepeatedField(GPBType::INT64);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetStringFail()
    {
        $arr = new RepeatedField(GPBType::INT64);
        $arr[] = 0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64AppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::INT64);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetMessageFail()
    {
        $arr = new RepeatedField(GPBType::INT64);
        $arr[] = 0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64AppendStringFail()
    {
        $arr = new RepeatedField(GPBType::UINT64);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetStringFail()
    {
        $arr = new RepeatedField(GPBType::UINT64);
        $arr[] = 0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64AppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::UINT64);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetMessageFail()
    {
        $arr = new RepeatedField(GPBType::UINT64);
        $arr[] = 0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatAppendStringFail()
    {
        $arr = new RepeatedField(GPBType::FLOAT);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatSetStringFail()
    {
        $arr = new RepeatedField(GPBType::FLOAT);
        $arr[] = 0.0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatAppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::FLOAT);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testFloatSetMessageFail()
    {
        $arr = new RepeatedField(GPBType::FLOAT);
        $arr[] = 0.0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleAppendStringFail()
    {
        $arr = new RepeatedField(GPBType::DOUBLE);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleSetStringFail()
    {
        $arr = new RepeatedField(GPBType::DOUBLE);
        $arr[] = 0.0;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleAppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::DOUBLE);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleSetMessageFail()
    {
        $arr = new RepeatedField(GPBType::DOUBLE);
        $arr[] = 0.0;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolAppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::BOOL);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolSetMessageFail()
    {
        $arr = new RepeatedField(GPBType::BOOL);
        $arr[] = true;
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringAppendMessageFail()
    {
        $arr = new RepeatedField(GPBType::STRING);
        $arr[] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetMessageFail()
    {
        $arr = new RepeatedField(GPBType::STRING);
        $arr[] = 'abc';
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringAppendInvalidUTF8Fail()
    {
        $arr = new RepeatedField(GPBType::STRING);
        $hex = hex2bin("ff");
        $arr[] = $hex;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetInvalidUTF8Fail()
    {
        $arr = new RepeatedField(GPBType::STRING);
        $arr[] = 'abc';
        $hex = hex2bin("ff");
        $arr[0] = $hex;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageAppendIntFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = 1;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetIntFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = new TestMessage_Sub;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageAppendStringFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetStringFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = new TestMessage_Sub;
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageAppendOtherMessageFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = new TestMessage;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageAppendNullFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $null = null;
        $arr[] = $null;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetNullFail()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage_Sub::class);
        $arr[] = new TestMessage_Sub();
        $null = null;
        $arr[0] = $null;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRemoveMiddleFail()
    {
        $arr = new RepeatedField(GPBType::INT32);

        $arr[] = 0;
        $arr[] = 1;
        $arr[] = 2;
        $this->assertSame(3, count($arr));

        unset($arr[1]);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testRemoveEmptyFail()
    {
        $arr = new RepeatedField(GPBType::INT32);

        unset($arr[0]);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageOffsetFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;
        $arr[new TestMessage_Sub()] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringOffsetFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;
        $arr['abc'] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testSetNonExistedOffsetFail()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[0] = 0;
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

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolFieldInvalidStringFail()
    {
        $m = new TestMessage();
        $m->setOptionalBool(new TestMessage());
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

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageMergeFromInvalidTypeFail()
    {
        $m = new TestMessage();
        $n = new TestMessage_Sub();
        $m->mergeFrom($n);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetStringKeyFail()
    {
        $arr = new MapField(GPBType::INT32, GPBType::INT32);
        $arr['abc'] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetStringValueFail()
    {
        $arr = new MapField(GPBType::INT32, GPBType::INT32);
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetMessageKeyFail()
    {
        $arr = new MapField(GPBType::INT32, GPBType::INT32);
        $arr[new TestMessage_Sub()] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt32SetMessageValueFail()
    {
        $arr = new MapField(GPBType::INT32, GPBType::INT32);
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetStringKeyFail()
    {
        $arr = new MapField(GPBType::UINT32, GPBType::UINT32);
        $arr['abc'] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetStringValueFail()
    {
        $arr = new MapField(GPBType::UINT32, GPBType::UINT32);
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetMessageKeyFail()
    {
        $arr = new MapField(GPBType::UINT32, GPBType::UINT32);
        $arr[new TestMessage_Sub()] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint32SetMessageValueFail()
    {
        $arr = new MapField(GPBType::UINT32, GPBType::UINT32);
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetStringKeyFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::INT64);
        $arr['abc'] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetStringValueFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::INT64);
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetMessageKeyFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::INT64);
        $arr[new TestMessage_Sub()] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInt64SetMessageValueFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::INT64);
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetStringKeyFail()
    {
        $arr = new MapField(GPBType::UINT64, GPBType::UINT64);
        $arr['abc'] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetStringValueFail()
    {
        $arr = new MapField(GPBType::UINT64, GPBType::UINT64);
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetMessageKeyFail()
    {
        $arr = new MapField(GPBType::UINT64, GPBType::UINT64);
        $arr[new TestMessage_Sub()] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testUint64SetMessageValueFail()
    {
        $arr = new MapField(GPBType::UINT64, GPBType::UINT64);
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleSetStringValueFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::DOUBLE);
        $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testDoubleSetMessageValueFail()
    {
        $arr = new MapField(GPBType::INT64, GPBType::DOUBLE);
        $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolSetMessageKeyFail()
    {
        $arr = new MapField(GPBType::BOOL, GPBType::BOOL);
        $arr[new TestMessage_Sub()] = true;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testBoolSetMessageValueFail()
    {
        $arr = new MapField(GPBType::BOOL, GPBType::BOOL);
        $arr[true] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetInvalidUTF8KeyFail()
    {
        $arr = new MapField(GPBType::STRING, GPBType::STRING);
        $arr[hex2bin("ff")] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetInvalidUTF8ValueFail()
    {
        $arr = new MapField(GPBType::STRING, GPBType::STRING);
        $arr['abc'] = hex2bin("ff");
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetMessageKeyFail()
    {
        $arr = new MapField(GPBType::STRING, GPBType::STRING);
        $arr[new TestMessage_Sub()] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testStringSetMessageValueFail()
    {
        $arr = new MapField(GPBType::STRING, GPBType::STRING);
        $arr['abc'] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetIntValueFail()
    {
       $arr =
           new MapField(GPBType::INT32, GPBType::MESSAGE, TestMessage::class);
       $arr[0] = 0;
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetStringValueFail()
    {
       $arr =
           new MapField(GPBType::INT32, GPBType::MESSAGE, TestMessage::class);
       $arr[0] = 'abc';
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetOtherMessageValueFail()
    {
       $arr =
           new MapField(GPBType::INT32, GPBType::MESSAGE, TestMessage::class);
       $arr[0] = new TestMessage_Sub();
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testMessageSetNullFail()
    {
       $arr =
           new MapField(GPBType::INT32, GPBType::MESSAGE, TestMessage::class);
       $null = NULL;
       $arr[0] = $null;
    }

}
