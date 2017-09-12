<?php

require_once('generated/NoNamespaceEnum.php');
require_once('generated/NoNamespaceMessage.php');
require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestEnum;
use Foo\TestIncludeNamespaceMessage;
use Foo\TestIncludePrefixMessage;
use Foo\TestMessage;
use Foo\TestMessage_Sub;
use Foo\TestReverseFieldOrder;
use Foo\testLowerCaseMessage;
use Foo\testLowerCaseEnum;
use Php\Test\TestNamespace;

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

        $b = $sub1->getB();
        $b[] = 102;
        $sub1->setB($b);

        $n->setOptionalMessage($sub1);

        // Repeated
        $repeatedInt32 = $n->getRepeatedInt32();
        $repeatedInt32[] = 200;
        $n->setRepeatedInt32($repeatedInt32);

        $repeatedString = $n->getRepeatedString();
        $repeatedString[] = 'abc';
        $n->setRepeatedString($repeatedString);

        $sub2 = new TestMessage_Sub();
        $sub2->setA(201);
        $repeatedMessage = $n->getRepeatedMessage();
        $repeatedMessage[] = $sub2;
        $n->setRepeatedMessage($repeatedMessage);

        // Map
        $mapInt32Int32 = $n->getMapInt32Int32();
        $mapInt32Int32[1] = 300;
        $mapInt32Int32[-62] = 301;
        $n->setMapInt32Int32($mapInt32Int32);

        $mapStringString = $n->getMapStringString();
        $mapStringString['def'] = 'def';
        $n->setMapStringString($mapStringString);

        $mapInt32Message = $n->getMapInt32Message();
        $mapInt32Message[1] = new TestMessage_Sub();
        $mapInt32Message[1]->setA(302);
        $mapInt32Message[2] = new TestMessage_Sub();
        $mapInt32Message[2]->setA(303);
        $n->setMapInt32Message($mapInt32Message);

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

        $repeatedMessage = $n->getRepeatedMessage();
        $repeatedMessage[0]->setA(-201);
        $n->setRepeatedMessage($repeatedMessage);
        $this->assertSame(201, $m->getRepeatedMessage()[2]->getA());

        $mapInt32Message = $n->getMapInt32Message();
        $mapInt32Message[1]->setA(-302);
        $n->setMapInt32Message($mapInt32Message);

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

    #########################################################
    # Test message/enum without namespace.
    #########################################################

    public function testMessageWithoutNamespace()
    {
        $m = new TestMessage();
        $sub = new NoNameSpaceMessage();
        $m->setOptionalNoNamespaceMessage($sub);
        $repeatedNoNamespaceMessage = $m->getRepeatedNoNamespaceMessage();
        $repeatedNoNamespaceMessage[] = new NoNameSpaceMessage();
        $m->setRepeatedNoNamespaceMessage($repeatedNoNamespaceMessage);

        $n = new NoNamespaceMessage();
        $n->setB(NoNamespaceMessage_NestedEnum::ZERO);
    }

    public function testEnumWithoutNamespace()
    {
        $m = new TestMessage();
        $m->setOptionalNoNamespaceEnum(NoNameSpaceEnum::VALUE_A);
        $repeatedNoNamespaceEnum = $m->getRepeatedNoNamespaceEnum();
        $repeatedNoNamespaceEnum[] = NoNameSpaceEnum::VALUE_A;
        $m->setRepeatedNoNamespaceEnum($repeatedNoNamespaceEnum);
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
    # Test message with given namespace.
    #########################################################

    public function testNamespaceMessage()
    {
        $m = new TestIncludeNamespaceMessage();

        $n = new TestNamespace();
        $n->setA(1);
        $m->setNamespaceMessage($n);
        $this->assertSame(1, $m->getNamespaceMessage()->getA());

        $n = new TestEmptyNamespace();
        $n->setA(1);
        $m->setEmptyNamespaceMessage($n);
        $this->assertSame(1, $m->getEmptyNamespaceMessage()->getA());
    }

    #########################################################
    # Test prefix for reserved words.
    #########################################################

    public function testPrefixForReservedWords()
    {
        $m = new \Foo\TestMessage_Empty();
        $m = new \Foo\PBEmpty();
        $m = new \PrefixEmpty();
        $m = new \Foo\PBARRAY();

        $m = new \Lower\PBabstract();
        $m = new \Lower\PBand();
        $m = new \Lower\PBarray();
        $m = new \Lower\PBas();
        $m = new \Lower\PBbreak();
        $m = new \Lower\PBcallable();
        $m = new \Lower\PBcase();
        $m = new \Lower\PBcatch();
        $m = new \Lower\PBclass();
        $m = new \Lower\PBclone();
        $m = new \Lower\PBconst();
        $m = new \Lower\PBcontinue();
        $m = new \Lower\PBdeclare();
        $m = new \Lower\PBdefault();
        $m = new \Lower\PBdie();
        $m = new \Lower\PBdo();
        $m = new \Lower\PBecho();
        $m = new \Lower\PBelse();
        $m = new \Lower\PBelseif();
        $m = new \Lower\PBempty();
        $m = new \Lower\PBenddeclare();
        $m = new \Lower\PBendfor();
        $m = new \Lower\PBendforeach();
        $m = new \Lower\PBendif();
        $m = new \Lower\PBendswitch();
        $m = new \Lower\PBendwhile();
        $m = new \Lower\PBeval();
        $m = new \Lower\PBexit();
        $m = new \Lower\PBextends();
        $m = new \Lower\PBfinal();
        $m = new \Lower\PBfor();
        $m = new \Lower\PBforeach();
        $m = new \Lower\PBfunction();
        $m = new \Lower\PBglobal();
        $m = new \Lower\PBgoto();
        $m = new \Lower\PBif();
        $m = new \Lower\PBimplements();
        $m = new \Lower\PBinclude();
        $m = new \Lower\PBinclude_once();
        $m = new \Lower\PBinstanceof();
        $m = new \Lower\PBinsteadof();
        $m = new \Lower\PBinterface();
        $m = new \Lower\PBisset();
        $m = new \Lower\PBlist();
        $m = new \Lower\PBnamespace();
        $m = new \Lower\PBnew();
        $m = new \Lower\PBor();
        $m = new \Lower\PBprint();
        $m = new \Lower\PBprivate();
        $m = new \Lower\PBprotected();
        $m = new \Lower\PBpublic();
        $m = new \Lower\PBrequire();
        $m = new \Lower\PBrequire_once();
        $m = new \Lower\PBreturn();
        $m = new \Lower\PBstatic();
        $m = new \Lower\PBswitch();
        $m = new \Lower\PBthrow();
        $m = new \Lower\PBtrait();
        $m = new \Lower\PBtry();
        $m = new \Lower\PBunset();
        $m = new \Lower\PBuse();
        $m = new \Lower\PBvar();
        $m = new \Lower\PBwhile();
        $m = new \Lower\PBxor();
        $m = new \Lower\PBint();
        $m = new \Lower\PBfloat();
        $m = new \Lower\PBbool();
        $m = new \Lower\PBstring();
        $m = new \Lower\PBtrue();
        $m = new \Lower\PBfalse();
        $m = new \Lower\PBnull();
        $m = new \Lower\PBvoid();
        $m = new \Lower\PBiterable();

        $m = new \Upper\PBABSTRACT();
        $m = new \Upper\PBAND();
        $m = new \Upper\PBARRAY();
        $m = new \Upper\PBAS();
        $m = new \Upper\PBBREAK();
        $m = new \Upper\PBCALLABLE();
        $m = new \Upper\PBCASE();
        $m = new \Upper\PBCATCH();
        $m = new \Upper\PBCLASS();
        $m = new \Upper\PBCLONE();
        $m = new \Upper\PBCONST();
        $m = new \Upper\PBCONTINUE();
        $m = new \Upper\PBDECLARE();
        $m = new \Upper\PBDEFAULT();
        $m = new \Upper\PBDIE();
        $m = new \Upper\PBDO();
        $m = new \Upper\PBECHO();
        $m = new \Upper\PBELSE();
        $m = new \Upper\PBELSEIF();
        $m = new \Upper\PBEMPTY();
        $m = new \Upper\PBENDDECLARE();
        $m = new \Upper\PBENDFOR();
        $m = new \Upper\PBENDFOREACH();
        $m = new \Upper\PBENDIF();
        $m = new \Upper\PBENDSWITCH();
        $m = new \Upper\PBENDWHILE();
        $m = new \Upper\PBEVAL();
        $m = new \Upper\PBEXIT();
        $m = new \Upper\PBEXTENDS();
        $m = new \Upper\PBFINAL();
        $m = new \Upper\PBFOR();
        $m = new \Upper\PBFOREACH();
        $m = new \Upper\PBFUNCTION();
        $m = new \Upper\PBGLOBAL();
        $m = new \Upper\PBGOTO();
        $m = new \Upper\PBIF();
        $m = new \Upper\PBIMPLEMENTS();
        $m = new \Upper\PBINCLUDE();
        $m = new \Upper\PBINCLUDE_ONCE();
        $m = new \Upper\PBINSTANCEOF();
        $m = new \Upper\PBINSTEADOF();
        $m = new \Upper\PBINTERFACE();
        $m = new \Upper\PBISSET();
        $m = new \Upper\PBLIST();
        $m = new \Upper\PBNAMESPACE();
        $m = new \Upper\PBNEW();
        $m = new \Upper\PBOR();
        $m = new \Upper\PBPRINT();
        $m = new \Upper\PBPRIVATE();
        $m = new \Upper\PBPROTECTED();
        $m = new \Upper\PBPUBLIC();
        $m = new \Upper\PBREQUIRE();
        $m = new \Upper\PBREQUIRE_ONCE();
        $m = new \Upper\PBRETURN();
        $m = new \Upper\PBSTATIC();
        $m = new \Upper\PBSWITCH();
        $m = new \Upper\PBTHROW();
        $m = new \Upper\PBTRAIT();
        $m = new \Upper\PBTRY();
        $m = new \Upper\PBUNSET();
        $m = new \Upper\PBUSE();
        $m = new \Upper\PBVAR();
        $m = new \Upper\PBWHILE();
        $m = new \Upper\PBXOR();
        $m = new \Upper\PBINT();
        $m = new \Upper\PBFLOAT();
        $m = new \Upper\PBBOOL();
        $m = new \Upper\PBSTRING();
        $m = new \Upper\PBTRUE();
        $m = new \Upper\PBFALSE();
        $m = new \Upper\PBNULL();
        $m = new \Upper\PBVOID();
        $m = new \Upper\PBITERABLE();

        $m = new \Lower_enum\PBabstract();
        $m = new \Lower_enum\PBand();
        $m = new \Lower_enum\PBarray();
        $m = new \Lower_enum\PBas();
        $m = new \Lower_enum\PBbreak();
        $m = new \Lower_enum\PBcallable();
        $m = new \Lower_enum\PBcase();
        $m = new \Lower_enum\PBcatch();
        $m = new \Lower_enum\PBclass();
        $m = new \Lower_enum\PBclone();
        $m = new \Lower_enum\PBconst();
        $m = new \Lower_enum\PBcontinue();
        $m = new \Lower_enum\PBdeclare();
        $m = new \Lower_enum\PBdefault();
        $m = new \Lower_enum\PBdie();
        $m = new \Lower_enum\PBdo();
        $m = new \Lower_enum\PBecho();
        $m = new \Lower_enum\PBelse();
        $m = new \Lower_enum\PBelseif();
        $m = new \Lower_enum\PBempty();
        $m = new \Lower_enum\PBenddeclare();
        $m = new \Lower_enum\PBendfor();
        $m = new \Lower_enum\PBendforeach();
        $m = new \Lower_enum\PBendif();
        $m = new \Lower_enum\PBendswitch();
        $m = new \Lower_enum\PBendwhile();
        $m = new \Lower_enum\PBeval();
        $m = new \Lower_enum\PBexit();
        $m = new \Lower_enum\PBextends();
        $m = new \Lower_enum\PBfinal();
        $m = new \Lower_enum\PBfor();
        $m = new \Lower_enum\PBforeach();
        $m = new \Lower_enum\PBfunction();
        $m = new \Lower_enum\PBglobal();
        $m = new \Lower_enum\PBgoto();
        $m = new \Lower_enum\PBif();
        $m = new \Lower_enum\PBimplements();
        $m = new \Lower_enum\PBinclude();
        $m = new \Lower_enum\PBinclude_once();
        $m = new \Lower_enum\PBinstanceof();
        $m = new \Lower_enum\PBinsteadof();
        $m = new \Lower_enum\PBinterface();
        $m = new \Lower_enum\PBisset();
        $m = new \Lower_enum\PBlist();
        $m = new \Lower_enum\PBnamespace();
        $m = new \Lower_enum\PBnew();
        $m = new \Lower_enum\PBor();
        $m = new \Lower_enum\PBprint();
        $m = new \Lower_enum\PBprivate();
        $m = new \Lower_enum\PBprotected();
        $m = new \Lower_enum\PBpublic();
        $m = new \Lower_enum\PBrequire();
        $m = new \Lower_enum\PBrequire_once();
        $m = new \Lower_enum\PBreturn();
        $m = new \Lower_enum\PBstatic();
        $m = new \Lower_enum\PBswitch();
        $m = new \Lower_enum\PBthrow();
        $m = new \Lower_enum\PBtrait();
        $m = new \Lower_enum\PBtry();
        $m = new \Lower_enum\PBunset();
        $m = new \Lower_enum\PBuse();
        $m = new \Lower_enum\PBvar();
        $m = new \Lower_enum\PBwhile();
        $m = new \Lower_enum\PBxor();
        $m = new \Lower_enum\PBint();
        $m = new \Lower_enum\PBfloat();
        $m = new \Lower_enum\PBbool();
        $m = new \Lower_enum\PBstring();
        $m = new \Lower_enum\PBtrue();
        $m = new \Lower_enum\PBfalse();
        $m = new \Lower_enum\PBnull();
        $m = new \Lower_enum\PBvoid();
        $m = new \Lower_enum\PBiterable();

        $m = new \Upper_enum\PBABSTRACT();
        $m = new \Upper_enum\PBAND();
        $m = new \Upper_enum\PBARRAY();
        $m = new \Upper_enum\PBAS();
        $m = new \Upper_enum\PBBREAK();
        $m = new \Upper_enum\PBCALLABLE();
        $m = new \Upper_enum\PBCASE();
        $m = new \Upper_enum\PBCATCH();
        $m = new \Upper_enum\PBCLASS();
        $m = new \Upper_enum\PBCLONE();
        $m = new \Upper_enum\PBCONST();
        $m = new \Upper_enum\PBCONTINUE();
        $m = new \Upper_enum\PBDECLARE();
        $m = new \Upper_enum\PBDEFAULT();
        $m = new \Upper_enum\PBDIE();
        $m = new \Upper_enum\PBDO();
        $m = new \Upper_enum\PBECHO();
        $m = new \Upper_enum\PBELSE();
        $m = new \Upper_enum\PBELSEIF();
        $m = new \Upper_enum\PBEMPTY();
        $m = new \Upper_enum\PBENDDECLARE();
        $m = new \Upper_enum\PBENDFOR();
        $m = new \Upper_enum\PBENDFOREACH();
        $m = new \Upper_enum\PBENDIF();
        $m = new \Upper_enum\PBENDSWITCH();
        $m = new \Upper_enum\PBENDWHILE();
        $m = new \Upper_enum\PBEVAL();
        $m = new \Upper_enum\PBEXIT();
        $m = new \Upper_enum\PBEXTENDS();
        $m = new \Upper_enum\PBFINAL();
        $m = new \Upper_enum\PBFOR();
        $m = new \Upper_enum\PBFOREACH();
        $m = new \Upper_enum\PBFUNCTION();
        $m = new \Upper_enum\PBGLOBAL();
        $m = new \Upper_enum\PBGOTO();
        $m = new \Upper_enum\PBIF();
        $m = new \Upper_enum\PBIMPLEMENTS();
        $m = new \Upper_enum\PBINCLUDE();
        $m = new \Upper_enum\PBINCLUDE_ONCE();
        $m = new \Upper_enum\PBINSTANCEOF();
        $m = new \Upper_enum\PBINSTEADOF();
        $m = new \Upper_enum\PBINTERFACE();
        $m = new \Upper_enum\PBISSET();
        $m = new \Upper_enum\PBLIST();
        $m = new \Upper_enum\PBNAMESPACE();
        $m = new \Upper_enum\PBNEW();
        $m = new \Upper_enum\PBOR();
        $m = new \Upper_enum\PBPRINT();
        $m = new \Upper_enum\PBPRIVATE();
        $m = new \Upper_enum\PBPROTECTED();
        $m = new \Upper_enum\PBPUBLIC();
        $m = new \Upper_enum\PBREQUIRE();
        $m = new \Upper_enum\PBREQUIRE_ONCE();
        $m = new \Upper_enum\PBRETURN();
        $m = new \Upper_enum\PBSTATIC();
        $m = new \Upper_enum\PBSWITCH();
        $m = new \Upper_enum\PBTHROW();
        $m = new \Upper_enum\PBTRAIT();
        $m = new \Upper_enum\PBTRY();
        $m = new \Upper_enum\PBUNSET();
        $m = new \Upper_enum\PBUSE();
        $m = new \Upper_enum\PBVAR();
        $m = new \Upper_enum\PBWHILE();
        $m = new \Upper_enum\PBXOR();
        $m = new \Upper_enum\PBINT();
        $m = new \Upper_enum\PBFLOAT();
        $m = new \Upper_enum\PBBOOL();
        $m = new \Upper_enum\PBSTRING();
        $m = new \Upper_enum\PBTRUE();
        $m = new \Upper_enum\PBFALSE();
        $m = new \Upper_enum\PBNULL();
        $m = new \Upper_enum\PBVOID();
        $m = new \Upper_enum\PBITERABLE();

        $m = \Lower_enum_value\NotAllowed::PBabstract;
        $m = \Lower_enum_value\NotAllowed::PBand;
        $m = \Lower_enum_value\NotAllowed::PBarray;
        $m = \Lower_enum_value\NotAllowed::PBas;
        $m = \Lower_enum_value\NotAllowed::PBbreak;
        $m = \Lower_enum_value\NotAllowed::PBcallable;
        $m = \Lower_enum_value\NotAllowed::PBcase;
        $m = \Lower_enum_value\NotAllowed::PBcatch;
        $m = \Lower_enum_value\NotAllowed::PBclass;
        $m = \Lower_enum_value\NotAllowed::PBclone;
        $m = \Lower_enum_value\NotAllowed::PBconst;
        $m = \Lower_enum_value\NotAllowed::PBcontinue;
        $m = \Lower_enum_value\NotAllowed::PBdeclare;
        $m = \Lower_enum_value\NotAllowed::PBdefault;
        $m = \Lower_enum_value\NotAllowed::PBdie;
        $m = \Lower_enum_value\NotAllowed::PBdo;
        $m = \Lower_enum_value\NotAllowed::PBecho;
        $m = \Lower_enum_value\NotAllowed::PBelse;
        $m = \Lower_enum_value\NotAllowed::PBelseif;
        $m = \Lower_enum_value\NotAllowed::PBempty;
        $m = \Lower_enum_value\NotAllowed::PBenddeclare;
        $m = \Lower_enum_value\NotAllowed::PBendfor;
        $m = \Lower_enum_value\NotAllowed::PBendforeach;
        $m = \Lower_enum_value\NotAllowed::PBendif;
        $m = \Lower_enum_value\NotAllowed::PBendswitch;
        $m = \Lower_enum_value\NotAllowed::PBendwhile;
        $m = \Lower_enum_value\NotAllowed::PBeval;
        $m = \Lower_enum_value\NotAllowed::PBexit;
        $m = \Lower_enum_value\NotAllowed::PBextends;
        $m = \Lower_enum_value\NotAllowed::PBfinal;
        $m = \Lower_enum_value\NotAllowed::PBfor;
        $m = \Lower_enum_value\NotAllowed::PBforeach;
        $m = \Lower_enum_value\NotAllowed::PBfunction;
        $m = \Lower_enum_value\NotAllowed::PBglobal;
        $m = \Lower_enum_value\NotAllowed::PBgoto;
        $m = \Lower_enum_value\NotAllowed::PBif;
        $m = \Lower_enum_value\NotAllowed::PBimplements;
        $m = \Lower_enum_value\NotAllowed::PBinclude;
        $m = \Lower_enum_value\NotAllowed::PBinclude_once;
        $m = \Lower_enum_value\NotAllowed::PBinstanceof;
        $m = \Lower_enum_value\NotAllowed::PBinsteadof;
        $m = \Lower_enum_value\NotAllowed::PBinterface;
        $m = \Lower_enum_value\NotAllowed::PBisset;
        $m = \Lower_enum_value\NotAllowed::PBlist;
        $m = \Lower_enum_value\NotAllowed::PBnamespace;
        $m = \Lower_enum_value\NotAllowed::PBnew;
        $m = \Lower_enum_value\NotAllowed::PBor;
        $m = \Lower_enum_value\NotAllowed::PBprint;
        $m = \Lower_enum_value\NotAllowed::PBprivate;
        $m = \Lower_enum_value\NotAllowed::PBprotected;
        $m = \Lower_enum_value\NotAllowed::PBpublic;
        $m = \Lower_enum_value\NotAllowed::PBrequire;
        $m = \Lower_enum_value\NotAllowed::PBrequire_once;
        $m = \Lower_enum_value\NotAllowed::PBreturn;
        $m = \Lower_enum_value\NotAllowed::PBstatic;
        $m = \Lower_enum_value\NotAllowed::PBswitch;
        $m = \Lower_enum_value\NotAllowed::PBthrow;
        $m = \Lower_enum_value\NotAllowed::PBtrait;
        $m = \Lower_enum_value\NotAllowed::PBtry;
        $m = \Lower_enum_value\NotAllowed::PBunset;
        $m = \Lower_enum_value\NotAllowed::PBuse;
        $m = \Lower_enum_value\NotAllowed::PBvar;
        $m = \Lower_enum_value\NotAllowed::PBwhile;
        $m = \Lower_enum_value\NotAllowed::PBxor;
        $m = \Lower_enum_value\NotAllowed::int;
        $m = \Lower_enum_value\NotAllowed::float;
        $m = \Lower_enum_value\NotAllowed::bool;
        $m = \Lower_enum_value\NotAllowed::string;
        $m = \Lower_enum_value\NotAllowed::true;
        $m = \Lower_enum_value\NotAllowed::false;
        $m = \Lower_enum_value\NotAllowed::null;
        $m = \Lower_enum_value\NotAllowed::void;
        $m = \Lower_enum_value\NotAllowed::iterable;

        $m = \Upper_enum_value\NotAllowed::PBABSTRACT;
        $m = \Upper_enum_value\NotAllowed::PBAND;
        $m = \Upper_enum_value\NotAllowed::PBARRAY;
        $m = \Upper_enum_value\NotAllowed::PBAS;
        $m = \Upper_enum_value\NotAllowed::PBBREAK;
        $m = \Upper_enum_value\NotAllowed::PBCALLABLE;
        $m = \Upper_enum_value\NotAllowed::PBCASE;
        $m = \Upper_enum_value\NotAllowed::PBCATCH;
        $m = \Upper_enum_value\NotAllowed::PBCLASS;
        $m = \Upper_enum_value\NotAllowed::PBCLONE;
        $m = \Upper_enum_value\NotAllowed::PBCONST;
        $m = \Upper_enum_value\NotAllowed::PBCONTINUE;
        $m = \Upper_enum_value\NotAllowed::PBDECLARE;
        $m = \Upper_enum_value\NotAllowed::PBDEFAULT;
        $m = \Upper_enum_value\NotAllowed::PBDIE;
        $m = \Upper_enum_value\NotAllowed::PBDO;
        $m = \Upper_enum_value\NotAllowed::PBECHO;
        $m = \Upper_enum_value\NotAllowed::PBELSE;
        $m = \Upper_enum_value\NotAllowed::PBELSEIF;
        $m = \Upper_enum_value\NotAllowed::PBEMPTY;
        $m = \Upper_enum_value\NotAllowed::PBENDDECLARE;
        $m = \Upper_enum_value\NotAllowed::PBENDFOR;
        $m = \Upper_enum_value\NotAllowed::PBENDFOREACH;
        $m = \Upper_enum_value\NotAllowed::PBENDIF;
        $m = \Upper_enum_value\NotAllowed::PBENDSWITCH;
        $m = \Upper_enum_value\NotAllowed::PBENDWHILE;
        $m = \Upper_enum_value\NotAllowed::PBEVAL;
        $m = \Upper_enum_value\NotAllowed::PBEXIT;
        $m = \Upper_enum_value\NotAllowed::PBEXTENDS;
        $m = \Upper_enum_value\NotAllowed::PBFINAL;
        $m = \Upper_enum_value\NotAllowed::PBFOR;
        $m = \Upper_enum_value\NotAllowed::PBFOREACH;
        $m = \Upper_enum_value\NotAllowed::PBFUNCTION;
        $m = \Upper_enum_value\NotAllowed::PBGLOBAL;
        $m = \Upper_enum_value\NotAllowed::PBGOTO;
        $m = \Upper_enum_value\NotAllowed::PBIF;
        $m = \Upper_enum_value\NotAllowed::PBIMPLEMENTS;
        $m = \Upper_enum_value\NotAllowed::PBINCLUDE;
        $m = \Upper_enum_value\NotAllowed::PBINCLUDE_ONCE;
        $m = \Upper_enum_value\NotAllowed::PBINSTANCEOF;
        $m = \Upper_enum_value\NotAllowed::PBINSTEADOF;
        $m = \Upper_enum_value\NotAllowed::PBINTERFACE;
        $m = \Upper_enum_value\NotAllowed::PBISSET;
        $m = \Upper_enum_value\NotAllowed::PBLIST;
        $m = \Upper_enum_value\NotAllowed::PBNAMESPACE;
        $m = \Upper_enum_value\NotAllowed::PBNEW;
        $m = \Upper_enum_value\NotAllowed::PBOR;
        $m = \Upper_enum_value\NotAllowed::PBPRINT;
        $m = \Upper_enum_value\NotAllowed::PBPRIVATE;
        $m = \Upper_enum_value\NotAllowed::PBPROTECTED;
        $m = \Upper_enum_value\NotAllowed::PBPUBLIC;
        $m = \Upper_enum_value\NotAllowed::PBREQUIRE;
        $m = \Upper_enum_value\NotAllowed::PBREQUIRE_ONCE;
        $m = \Upper_enum_value\NotAllowed::PBRETURN;
        $m = \Upper_enum_value\NotAllowed::PBSTATIC;
        $m = \Upper_enum_value\NotAllowed::PBSWITCH;
        $m = \Upper_enum_value\NotAllowed::PBTHROW;
        $m = \Upper_enum_value\NotAllowed::PBTRAIT;
        $m = \Upper_enum_value\NotAllowed::PBTRY;
        $m = \Upper_enum_value\NotAllowed::PBUNSET;
        $m = \Upper_enum_value\NotAllowed::PBUSE;
        $m = \Upper_enum_value\NotAllowed::PBVAR;
        $m = \Upper_enum_value\NotAllowed::PBWHILE;
        $m = \Upper_enum_value\NotAllowed::PBXOR;
        $m = \Upper_enum_value\NotAllowed::INT;
        $m = \Upper_enum_value\NotAllowed::FLOAT;
        $m = \Upper_enum_value\NotAllowed::BOOL;
        $m = \Upper_enum_value\NotAllowed::STRING;
        $m = \Upper_enum_value\NotAllowed::TRUE;
        $m = \Upper_enum_value\NotAllowed::FALSE;
        $m = \Upper_enum_value\NotAllowed::NULL;
        $m = \Upper_enum_value\NotAllowed::VOID;
        $m = \Upper_enum_value\NotAllowed::ITERABLE;
    }

    #########################################################
    # Test fluent setters.
    #########################################################

    public function testFluentSetters()
    {
        $m = (new TestMessage())
            ->setOptionalInt32(1)
            ->setOptionalUInt32(2);
        $this->assertSame(1, $m->getOptionalInt32());
        $this->assertSame(2, $m->getOptionalUInt32());
    }

    #########################################################
    # Test Reverse Field Order.
    #########################################################

    public function testReverseFieldOrder()
    {
        $m = new TestReverseFieldOrder();
        $m->setB("abc");
        $this->assertSame("abc", $m->getB());
        $this->assertNotSame("abc", $m->getA());
    }

    #########################################################
    # Test Reverse Field Order.
    #########################################################

    public function testLowerCase()
    {
        $m = new testLowerCaseMessage();
        $n = testLowerCaseEnum::VALUE;
    }
}
