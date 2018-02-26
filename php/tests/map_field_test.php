<?php

require_once('test_util.php');

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\MapField;
use Foo\TestMessage;
use Foo\TestMessage_Sub;

class MapFieldTest extends PHPUnit_Framework_TestCase {

    #########################################################
    # Test int32 field.
    #########################################################

    public function testInt32() {
        $arr = new MapField(GPBType::INT32, GPBType::INT32);

        // Test integer argument.
        $arr[MAX_INT32] = MAX_INT32;
        $this->assertSame(MAX_INT32, $arr[MAX_INT32]);
        $arr[MIN_INT32] = MIN_INT32;
        $this->assertSame(MIN_INT32, $arr[MIN_INT32]);
        $this->assertCount(2, $arr);
        $this->assertArrayHasKey(MAX_INT32, $arr);
        $this->assertArrayHasKey(MIN_INT32, $arr);
        unset($arr[MAX_INT32]);
        unset($arr[MIN_INT32]);
        $this->assertCount(0, $arr);

        // Test float argument.
        $arr[1.9] = 1.9;
        $arr[2.1] = 2.1;
        $this->assertSame(1, $arr[1]);
        $this->assertSame(2, $arr[2]);
        $arr[MAX_INT32_FLOAT] = MAX_INT32_FLOAT;
        $this->assertSame(MAX_INT32, $arr[MAX_INT32]);
        $arr[MIN_INT32_FLOAT] = MIN_INT32_FLOAT;
        $this->assertSame(MIN_INT32, $arr[MIN_INT32]);
        $this->assertCount(4, $arr);
        unset($arr[1.9]);
        unset($arr[2.9]);
        unset($arr[MAX_INT32_FLOAT]);
        unset($arr[MIN_INT32_FLOAT]);
        $this->assertCount(0, $arr);

        // Test string argument.
        $arr['2'] = '2';
        $this->assertSame(2, $arr[2]);
        $arr['3.1'] = '3.1';
        $this->assertSame(3, $arr[3]);
        $arr[MAX_INT32_STRING] = MAX_INT32_STRING;
        $this->assertSame(MAX_INT32, $arr[MAX_INT32]);
        $this->assertCount(3, $arr);
        unset($arr['2']);
        unset($arr['3.1']);
        unset($arr[MAX_INT32_STRING]);
        $this->assertCount(0, $arr);

        // Test foreach.
        $arr = new MapField(GPBType::INT32, GPBType::INT32);
        for ($i = 0; $i < 3; $i++) {
          $arr[$i] = $i;
        }
        $i = 0;
        $arr_test = [];
        foreach ($arr as $key => $val) {
          $this->assertSame($key, $val);
          $arr_test[] = $key;
          $i++;
        }
        $this->assertArrayHasKey(0, $arr_test);
        $this->assertArrayHasKey(1, $arr_test);
        $this->assertArrayHasKey(2, $arr_test);
        $this->assertSame(3, $i);
    }

    #########################################################
    # Test uint32 field.
    #########################################################

    public function testUint32() {
        $arr = new MapField(GPBType::UINT32, GPBType::UINT32);

        // Test integer argument.
        $arr[MAX_UINT32] = MAX_UINT32;
        $this->assertSame(-1, $arr[-1]);
        $this->assertCount(1, $arr);
        unset($arr[MAX_UINT32]);
        $this->assertCount(0, $arr);

        $arr[-1] = -1;
        $this->assertSame(-1, $arr[-1]);
        $arr[MIN_UINT32] = MIN_UINT32;
        $this->assertSame(MIN_UINT32, $arr[MIN_UINT32]);
        $this->assertCount(2, $arr);
        unset($arr[-1]);
        unset($arr[MIN_UINT32]);
        $this->assertCount(0, $arr);

        // Test float argument.
        $arr[MAX_UINT32_FLOAT] = MAX_UINT32_FLOAT;
        $this->assertSame(-1, $arr[-1]);
        $this->assertCount(1, $arr);
        unset($arr[MAX_UINT32_FLOAT]);
        $this->assertCount(0, $arr);

        $arr[3.1] = 3.1;
        $this->assertSame(3, $arr[3]);
        $arr[-1.0] = -1.0;
        $this->assertSame(-1, $arr[-1]);
        $arr[MIN_UINT32_FLOAT] = MIN_UINT32_FLOAT;
        $this->assertSame(MIN_UINT32, $arr[MIN_UINT32]);
        $this->assertCount(3, $arr);
        unset($arr[3.1]);
        unset($arr[-1.0]);
        unset($arr[MIN_UINT32_FLOAT]);
        $this->assertCount(0, $arr);

        // Test string argument.
        $arr[MAX_UINT32_STRING] = MAX_UINT32_STRING;
        $this->assertSame(-1, $arr[-1]);
        $this->assertCount(1, $arr);
        unset($arr[MAX_UINT32_STRING]);
        $this->assertCount(0, $arr);

        $arr['7'] = '7';
        $this->assertSame(7, $arr[7]);
        $arr['3.1'] = '3.1';
        $this->assertSame(3, $arr[3]);
        $arr['-1.0'] = '-1.0';
        $this->assertSame(-1, $arr[-1]);
        $arr[MIN_UINT32_STRING] = MIN_UINT32_STRING;
        $this->assertSame(MIN_UINT32, $arr[MIN_UINT32]);
        $this->assertCount(4, $arr);
        unset($arr['7']);
        unset($arr['3.1']);
        unset($arr['-1.0']);
        unset($arr[MIN_UINT32_STRING]);
        $this->assertCount(0, $arr);
    }

    #########################################################
    # Test int64 field.
    #########################################################

    public function testInt64() {
        $arr = new MapField(GPBType::INT64, GPBType::INT64);

        // Test integer argument.
        $arr[MAX_INT64] = MAX_INT64;
        $arr[MIN_INT64] = MIN_INT64;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_INT64_STRING, $arr[MAX_INT64_STRING]);
            $this->assertSame(MIN_INT64_STRING, $arr[MIN_INT64_STRING]);
        } else {
            $this->assertSame(MAX_INT64, $arr[MAX_INT64]);
            $this->assertSame(MIN_INT64, $arr[MIN_INT64]);
        }
        $this->assertCount(2, $arr);
        unset($arr[MAX_INT64]);
        unset($arr[MIN_INT64]);
        $this->assertCount(0, $arr);

        // Test float argument.
        $arr[1.1] = 1.1;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('1', $arr['1']);
        } else {
            $this->assertSame(1, $arr[1]);
        }
        $this->assertCount(1, $arr);
        unset($arr[1.1]);
        $this->assertCount(0, $arr);

        // Test string argument.
        $arr['2'] = '2';
        $arr['3.1'] = '3.1';
        $arr[MAX_INT64_STRING] = MAX_INT64_STRING;
        $arr[MIN_INT64_STRING] = MIN_INT64_STRING;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('2', $arr['2']);
            $this->assertSame('3', $arr['3']);
            $this->assertSame(MAX_INT64_STRING, $arr[MAX_INT64_STRING]);
            $this->assertSame(MIN_INT64_STRING, $arr[MIN_INT64_STRING]);
        } else {
            $this->assertSame(2, $arr[2]);
            $this->assertSame(3, $arr[3]);
            $this->assertSame(MAX_INT64, $arr[MAX_INT64]);
            $this->assertSame(MIN_INT64, $arr[MIN_INT64]);
        }
        $this->assertCount(4, $arr);
        unset($arr['2']);
        unset($arr['3.1']);
        unset($arr[MAX_INT64_STRING]);
        unset($arr[MIN_INT64_STRING]);
        $this->assertCount(0, $arr);
    }

    #########################################################
    # Test uint64 field.
    #########################################################

    public function testUint64() {
        $arr = new MapField(GPBType::UINT64, GPBType::UINT64);

        // Test integer argument.
        $arr[MAX_UINT64] = MAX_UINT64;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_UINT64_STRING, $arr[MAX_UINT64_STRING]);
        } else {
            $this->assertSame(MAX_UINT64, $arr[MAX_UINT64]);
        }
        $this->assertCount(1, $arr);
        unset($arr[MAX_UINT64]);
        $this->assertCount(0, $arr);

        // Test float argument.
        $arr[1.1] = 1.1;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('1', $arr['1']);
        } else {
            $this->assertSame(1, $arr[1]);
        }
        $this->assertCount(1, $arr);
        unset($arr[1.1]);
        $this->assertCount(0, $arr);

        // Test string argument.
        $arr['2'] = '2';
        $arr['3.1'] = '3.1';
        $arr[MAX_UINT64_STRING] = MAX_UINT64_STRING;

        if (PHP_INT_SIZE == 4) {
            $this->assertSame('2', $arr['2']);
            $this->assertSame('3', $arr['3']);
            $this->assertSame(MAX_UINT64_STRING, $arr[MAX_UINT64_STRING]);
        } else {
            $this->assertSame(2, $arr[2]);
            $this->assertSame(3, $arr[3]);
            $this->assertSame(MAX_UINT64, $arr[MAX_UINT64]);
        }

        $this->assertCount(3, $arr);
        unset($arr['2']);
        unset($arr['3.1']);
        unset($arr[MAX_UINT64_STRING]);
        $this->assertCount(0, $arr);
    }

    #########################################################
    # Test float field.
    #########################################################

    public function testFloat() {
        $arr = new MapField(GPBType::INT32, GPBType::FLOAT);

        // Test set.
        $arr[0] = 1;
        $this->assertEquals(1.0, $arr[0], '', MAX_FLOAT_DIFF);

        $arr[1] = 1.1;
        $this->assertEquals(1.1, $arr[1], '', MAX_FLOAT_DIFF);

        $arr[2] = '2';
        $this->assertEquals(2.0, $arr[2], '', MAX_FLOAT_DIFF);
        $arr[3] = '3.1';
        $this->assertEquals(3.1, $arr[3], '', MAX_FLOAT_DIFF);

        $this->assertCount(4, $arr);
    }

    #########################################################
    # Test double field.
    #########################################################

    public function testDouble() {
        $arr = new MapField(GPBType::INT32, GPBType::DOUBLE);

        // Test set.
        $arr[0] = 1;
        $this->assertEquals(1.0, $arr[0], '', MAX_FLOAT_DIFF);

        $arr[1] = 1.1;
        $this->assertEquals(1.1, $arr[1], '', MAX_FLOAT_DIFF);

        $arr[2] = '2';
        $this->assertEquals(2.0, $arr[2], '', MAX_FLOAT_DIFF);
        $arr[3] = '3.1';
        $this->assertEquals(3.1, $arr[3], '', MAX_FLOAT_DIFF);

        $this->assertCount(4, $arr);
    }

    #########################################################
    # Test bool field.
    #########################################################

    public function testBool() {
        $arr = new MapField(GPBType::BOOL, GPBType::BOOL);

        // Test boolean.
        $arr[True] = True;
        $this->assertTrue($arr[True]);
        $this->assertCount(1, $arr);
        unset($arr[True]);
        $this->assertCount(0, $arr);

        $arr[False] = False;
        $this->assertFalse($arr[False]);
        $this->assertCount(1, $arr);
        unset($arr[False]);
        $this->assertCount(0, $arr);

        // Test integer.
        $arr[-1] = -1;
        $this->assertTrue($arr[True]);
        $this->assertCount(1, $arr);
        unset($arr[-1]);
        $this->assertCount(0, $arr);

        $arr[0] = 0;
        $this->assertFalse($arr[False]);
        $this->assertCount(1, $arr);
        unset($arr[0]);
        $this->assertCount(0, $arr);

        // Test float.
        $arr[1.1] = 1.1;
        $this->assertTrue($arr[True]);
        $this->assertCount(1, $arr);
        unset($arr[1.1]);
        $this->assertCount(0, $arr);

        $arr[0.0] = 0.0;
        $this->assertFalse($arr[False]);
        $this->assertCount(1, $arr);
        unset($arr[0.0]);
        $this->assertCount(0, $arr);

        // Test string.
        $arr['a'] = 'a';
        $this->assertTrue($arr[True]);
        $this->assertCount(1, $arr);
        unset($arr['a']);
        $this->assertCount(0, $arr);

        $arr[''] = '';
        $this->assertFalse($arr[False]);
        $this->assertCount(1, $arr);
        unset($arr['']);
        $this->assertCount(0, $arr);
    }

    #########################################################
    # Test string field.
    #########################################################

    public function testString() {
        $arr = new MapField(GPBType::STRING, GPBType::STRING);

        // Test set.
        $arr['abc'] = 'abc';
        $this->assertSame('abc', $arr['abc']);
        $this->assertCount(1, $arr);
        unset($arr['abc']);
        $this->assertCount(0, $arr);

        $arr[1] = 1;
        $this->assertSame('1', $arr['1']);
        $this->assertCount(1, $arr);
        unset($arr[1]);
        $this->assertCount(0, $arr);

        $arr[1.1] = 1.1;
        $this->assertSame('1.1', $arr['1.1']);
        $this->assertCount(1, $arr);
        unset($arr[1.1]);
        $this->assertCount(0, $arr);

        $arr[True] = True;
        $this->assertSame('1', $arr['1']);
        $this->assertCount(1, $arr);
        unset($arr[True]);
        $this->assertCount(0, $arr);

        // Test foreach.
        $arr = new MapField(GPBType::STRING, GPBType::STRING);
        for ($i = 0; $i < 3; $i++) {
          $arr[$i] = $i;
        }
        $i = 0;
        $arr_test = [];
        foreach ($arr as $key => $val) {
          $this->assertSame($key, $val);
          $arr_test[] = $key;
          $i++;
        }
        $this->assertArrayHasKey('0', $arr_test);
        $this->assertArrayHasKey('1', $arr_test);
        $this->assertArrayHasKey('2', $arr_test);
        $this->assertSame(3, $i);
    }

    #########################################################
    # Test message field.
    #########################################################

    public function testMessage() {
        $arr = new MapField(GPBType::INT32,
            GPBType::MESSAGE, TestMessage_Sub::class);

        // Test append.
        $sub_m = new TestMessage_Sub();
        $sub_m->setA(1);
        $arr[0] = $sub_m;
        $this->assertSame(1, $arr[0]->getA());

        $this->assertCount(1, $arr);

        // Test foreach.
        $arr = new MapField(GPBType::INT32,
            GPBType::MESSAGE, TestMessage_Sub::class);
        for ($i = 0; $i < 3; $i++) {
          $arr[$i] = new TestMessage_Sub();;
          $arr[$i]->setA($i);
        }
        $i = 0;
        $key_test = [];
        $value_test = [];
        foreach ($arr as $key => $val) {
          $key_test[] = $key;
          $value_test[] = $val->getA();
          $i++;
        }
        $this->assertArrayHasKey('0', $key_test);
        $this->assertArrayHasKey('1', $key_test);
        $this->assertArrayHasKey('2', $key_test);
        $this->assertArrayHasKey('0', $value_test);
        $this->assertArrayHasKey('1', $value_test);
        $this->assertArrayHasKey('2', $value_test);
        $this->assertSame(3, $i);
    }

    #########################################################
    # Test memory leak
    #########################################################

    // TODO(teboring): Add it back.
    // public function testCycleLeak()
    // {
    //     $arr = new MapField(GPBType::INT32,
    //         GPBType::MESSAGE, TestMessage::class);
    //     $arr[0] = new TestMessage;
    //     $arr[0]->SetMapRecursive($arr);

    //     // Clean up memory before test.
    //     gc_collect_cycles();
    //     $start = memory_get_usage();
    //     unset($arr);

    //     // Explicitly trigger garbage collection.
    //     gc_collect_cycles();

    //     $end = memory_get_usage();
    //     $this->assertLessThan($start, $end);
    // }
}
