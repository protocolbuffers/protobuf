<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestMessage;
use Foo\TestMessage\Sub;

class ArrayTest extends TestBase
{

    #########################################################
    # Test int32 field.
    #########################################################

    public function testInt32()
    {
        $arr = new RepeatedField(GPBType::INT32);

        // Test append.
        $arr[] = MAX_INT32;
        $this->assertSame(MAX_INT32, $arr[0]);
        $arr[] = MIN_INT32;
        $this->assertSame(MIN_INT32, $arr[1]);

        $arr[] = 1.1;
        $this->assertSame(1, $arr[2]);
        $arr[] = MAX_INT32_FLOAT;
        $this->assertSame(MAX_INT32, $arr[3]);
        $arr[] = MAX_INT32_FLOAT;
        $this->assertSame(MAX_INT32, $arr[4]);

        $arr[] = '2';
        $this->assertSame(2, $arr[5]);
        $arr[] = '3.1';
        $this->assertSame(3, $arr[6]);
        $arr[] = MAX_INT32_STRING;
        $this->assertSame(MAX_INT32, $arr[7]);

        $this->assertEquals(8, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            $this->assertSame(0, $arr[$i]);
        }

        // Test set.
        $arr[0] = MAX_INT32;
        $this->assertSame(MAX_INT32, $arr[0]);
        $arr[1] = MIN_INT32;
        $this->assertSame(MIN_INT32, $arr[1]);

        $arr[2] = 1.1;
        $this->assertSame(1, $arr[2]);
        $arr[3] = MAX_INT32_FLOAT;
        $this->assertSame(MAX_INT32, $arr[3]);
        $arr[4] = MAX_INT32_FLOAT;
        $this->assertSame(MAX_INT32, $arr[4]);

        $arr[5] = '2';
        $this->assertSame(2, $arr[5]);
        $arr[6] = '3.1';
        $this->assertSame(3, $arr[6]);
        $arr[7] = MAX_INT32_STRING;
        $this->assertSame(MAX_INT32, $arr[7]);

        // Test foreach.
        $arr = new RepeatedField(GPBType::INT32);
        for ($i = 0; $i < 3; $i++) {
          $arr[] = $i;
        }
        $i = 0;
        foreach ($arr as $val) {
          $this->assertSame($i++, $val);
        }
        $this->assertSame(3, $i);
    }

    #########################################################
    # Test uint32 field.
    #########################################################

    public function testUint32()
    {
        $arr = new RepeatedField(GPBType::UINT32);

        // Test append.
        $arr[] = MAX_UINT32;
        $this->assertSame(-1, $arr[0]);
        $arr[] = -1;
        $this->assertSame(-1, $arr[1]);
        $arr[] = MIN_UINT32;
        $this->assertSame(MIN_UINT32, $arr[2]);

        $arr[] = 1.1;
        $this->assertSame(1, $arr[3]);
        $arr[] = MAX_UINT32_FLOAT;
        $this->assertSame(-1, $arr[4]);
        $arr[] = -1.0;
        $this->assertSame(-1, $arr[5]);
        $arr[] = MIN_UINT32_FLOAT;
        $this->assertSame(MIN_UINT32, $arr[6]);

        $arr[] = '2';
        $this->assertSame(2, $arr[7]);
        $arr[] = '3.1';
        $this->assertSame(3, $arr[8]);
        $arr[] = MAX_UINT32_STRING;
        $this->assertSame(-1, $arr[9]);
        $arr[] = '-1.0';
        $this->assertSame(-1, $arr[10]);
        $arr[] = MIN_UINT32_STRING;
        $this->assertSame(MIN_UINT32, $arr[11]);

        $this->assertEquals(12, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            $this->assertSame(0, $arr[$i]);
        }

        // Test set.
        $arr[0] = MAX_UINT32;
        $this->assertSame(-1, $arr[0]);
        $arr[1] = -1;
        $this->assertSame(-1, $arr[1]);
        $arr[2] = MIN_UINT32;
        $this->assertSame(MIN_UINT32, $arr[2]);

        $arr[3] = 1.1;
        $this->assertSame(1, $arr[3]);
        $arr[4] = MAX_UINT32_FLOAT;
        $this->assertSame(-1, $arr[4]);
        $arr[5] = -1.0;
        $this->assertSame(-1, $arr[5]);
        $arr[6] = MIN_UINT32_FLOAT;
        $this->assertSame(MIN_UINT32, $arr[6]);

        $arr[7] = '2';
        $this->assertSame(2, $arr[7]);
        $arr[8] = '3.1';
        $this->assertSame(3, $arr[8]);
        $arr[9] = MAX_UINT32_STRING;
        $this->assertSame(-1, $arr[9]);
        $arr[10] = '-1.0';
        $this->assertSame(-1, $arr[10]);
        $arr[11] = MIN_UINT32_STRING;
        $this->assertSame(MIN_UINT32, $arr[11]);
    }

    #########################################################
    # Test int64 field.
    #########################################################

    public function testInt64()
    {
        $arr = new RepeatedField(GPBType::INT64);

        // Test append.
        $arr[] = MAX_INT64;
        $arr[] = MIN_INT64;
        $arr[] = 1.1;
        $arr[] = '2';
        $arr[] = '3.1';
        $arr[] = MAX_INT64_STRING;
        $arr[] = MIN_INT64_STRING;
        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_INT64, $arr[0]);
            $this->assertSame(MIN_INT64, $arr[1]);
            $this->assertSame('1', $arr[2]);
            $this->assertSame('2', $arr[3]);
            $this->assertSame('3', $arr[4]);
            $this->assertSame(MAX_INT64_STRING, $arr[5]);
            $this->assertSame(MIN_INT64_STRING, $arr[6]);
        } else {
            $this->assertSame(MAX_INT64, $arr[0]);
            $this->assertSame(MIN_INT64, $arr[1]);
            $this->assertSame(1, $arr[2]);
            $this->assertSame(2, $arr[3]);
            $this->assertSame(3, $arr[4]);
            $this->assertSame(MAX_INT64, $arr[5]);
            $this->assertSame(MIN_INT64, $arr[6]);
        }


        $this->assertEquals(7, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            if (PHP_INT_SIZE == 4) {
                $this->assertSame('0', $arr[$i]);
            } else {
                $this->assertSame(0, $arr[$i]);
            }
        }

        // Test set.
        $arr[0] = MAX_INT64;
        $arr[1] = MIN_INT64;
        $arr[2] = 1.1;
        $arr[3] = '2';
        $arr[4] = '3.1';
        $arr[5] = MAX_INT64_STRING;
        $arr[6] = MIN_INT64_STRING;

        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_INT64_STRING, $arr[0]);
            $this->assertSame(MIN_INT64_STRING, $arr[1]);
            $this->assertSame('1', $arr[2]);
            $this->assertSame('2', $arr[3]);
            $this->assertSame('3', $arr[4]);
            $this->assertSame(MAX_INT64_STRING, $arr[5]);
            $this->assertEquals(MIN_INT64_STRING, $arr[6]);
        } else {
            $this->assertSame(MAX_INT64, $arr[0]);
            $this->assertSame(MIN_INT64, $arr[1]);
            $this->assertSame(1, $arr[2]);
            $this->assertSame(2, $arr[3]);
            $this->assertSame(3, $arr[4]);
            $this->assertSame(MAX_INT64, $arr[5]);
            $this->assertEquals(MIN_INT64, $arr[6]);
        }
    }

    #########################################################
    # Test uint64 field.
    #########################################################

    public function testUint64()
    {
        $arr = new RepeatedField(GPBType::UINT64);

        // Test append.
        $arr[] = MAX_UINT64;
        $arr[] = 1.1;
        $arr[] = '2';
        $arr[] = '3.1';
        $arr[] = MAX_UINT64_STRING;

        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_UINT64_STRING, $arr[0]);
            $this->assertSame('1', $arr[1]);
            $this->assertSame('2', $arr[2]);
            $this->assertSame('3', $arr[3]);
            $this->assertSame(MAX_UINT64_STRING, $arr[4]);
        } else {
            $this->assertSame(MAX_UINT64, $arr[0]);
            $this->assertSame(1, $arr[1]);
            $this->assertSame(2, $arr[2]);
            $this->assertSame(3, $arr[3]);
            $this->assertSame(MAX_UINT64, $arr[4]);
            $this->assertSame(5, count($arr));
        }

        $this->assertSame(5, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            if (PHP_INT_SIZE == 4) {
                $this->assertSame('0', $arr[$i]);
            } else {
                $this->assertSame(0, $arr[$i]);
            }
        }

        // Test set.
        $arr[0] = MAX_UINT64;
        $arr[1] = 1.1;
        $arr[2] = '2';
        $arr[3] = '3.1';
        $arr[4] = MAX_UINT64_STRING;

        if (PHP_INT_SIZE == 4) {
            $this->assertSame(MAX_UINT64_STRING, $arr[0]);
            $this->assertSame('1', $arr[1]);
            $this->assertSame('2', $arr[2]);
            $this->assertSame('3', $arr[3]);
            $this->assertSame(MAX_UINT64_STRING, $arr[4]);
        } else {
            $this->assertSame(MAX_UINT64, $arr[0]);
            $this->assertSame(1, $arr[1]);
            $this->assertSame(2, $arr[2]);
            $this->assertSame(3, $arr[3]);
            $this->assertSame(MAX_UINT64, $arr[4]);
        }
    }

    #########################################################
    # Test float field.
    #########################################################

    public function testFloat()
    {
        $arr = new RepeatedField(GPBType::FLOAT);

        // Test append.
        $arr[] = 1;
        $this->assertFloatEquals(1.0, $arr[0], MAX_FLOAT_DIFF);

        $arr[] = 1.1;
        $this->assertFloatEquals(1.1, $arr[1], MAX_FLOAT_DIFF);

        $arr[] = '2';
        $this->assertFloatEquals(2.0, $arr[2], MAX_FLOAT_DIFF);
        $arr[] = '3.1';
        $this->assertFloatEquals(3.1, $arr[3], MAX_FLOAT_DIFF);

        $this->assertEquals(4, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            $this->assertSame(0.0, $arr[$i]);
        }

        // Test set.
        $arr[0] = 1;
        $this->assertFloatEquals(1.0, $arr[0], MAX_FLOAT_DIFF);

        $arr[1] = 1.1;
        $this->assertFloatEquals(1.1, $arr[1], MAX_FLOAT_DIFF);

        $arr[2] = '2';
        $this->assertFloatEquals(2.0, $arr[2], MAX_FLOAT_DIFF);
        $arr[3] = '3.1';
        $this->assertFloatEquals(3.1, $arr[3], MAX_FLOAT_DIFF);
    }

    #########################################################
    # Test double field.
    #########################################################

    public function testDouble()
    {
        $arr = new RepeatedField(GPBType::DOUBLE);

        // Test append.
        $arr[] = 1;
        $this->assertFloatEquals(1.0, $arr[0], MAX_FLOAT_DIFF);

        $arr[] = 1.1;
        $this->assertFloatEquals(1.1, $arr[1], MAX_FLOAT_DIFF);

        $arr[] = '2';
        $this->assertFloatEquals(2.0, $arr[2], MAX_FLOAT_DIFF);
        $arr[] = '3.1';
        $this->assertFloatEquals(3.1, $arr[3], MAX_FLOAT_DIFF);

        $this->assertEquals(4, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            $this->assertSame(0.0, $arr[$i]);
        }

        // Test set.
        $arr[0] = 1;
        $this->assertFloatEquals(1.0, $arr[0], MAX_FLOAT_DIFF);

        $arr[1] = 1.1;
        $this->assertFloatEquals(1.1, $arr[1], MAX_FLOAT_DIFF);

        $arr[2] = '2';
        $this->assertFloatEquals(2.0, $arr[2], MAX_FLOAT_DIFF);
        $arr[3] = '3.1';
        $this->assertFloatEquals(3.1, $arr[3], MAX_FLOAT_DIFF);
    }

    #########################################################
    # Test bool field.
    #########################################################

    public function testBool()
    {
        $arr = new RepeatedField(GPBType::BOOL);

        // Test append.
        $arr[] = true;
        $this->assertSame(true, $arr[0]);

        $arr[] = -1;
        $this->assertSame(true, $arr[1]);

        $arr[] = 1.1;
        $this->assertSame(true, $arr[2]);

        $arr[] = '';
        $this->assertSame(false, $arr[3]);

        $this->assertEquals(4, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = 0;
            $this->assertSame(false, $arr[$i]);
        }

        // Test set.
        $arr[0] = true;
        $this->assertSame(true, $arr[0]);

        $arr[1] = -1;
        $this->assertSame(true, $arr[1]);

        $arr[2] = 1.1;
        $this->assertSame(true, $arr[2]);

        $arr[3] = '';
        $this->assertSame(false, $arr[3]);
    }

    #########################################################
    # Test string field.
    #########################################################

    public function testString()
    {
        $arr = new RepeatedField(GPBType::STRING);

        // Test append.
        $arr[] = 'abc';
        $this->assertSame('abc', $arr[0]);

        $arr[] = 1;
        $this->assertSame('1', $arr[1]);

        $arr[] = 1.1;
        $this->assertSame('1.1', $arr[2]);

        $arr[] = true;
        $this->assertSame('1', $arr[3]);

        $this->assertEquals(4, count($arr));

        for ($i = 0; $i < count($arr); $i++) {
            $arr[$i] = '';
            $this->assertSame('', $arr[$i]);
        }

        // Test set.
        $arr[0] = 'abc';
        $this->assertSame('abc', $arr[0]);

        $arr[1] = 1;
        $this->assertSame('1', $arr[1]);

        $arr[2] = 1.1;
        $this->assertSame('1.1', $arr[2]);

        $arr[3] = true;
        $this->assertSame('1', $arr[3]);
    }

    #########################################################
    # Test message field.
    #########################################################

    public function testMessage()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, Sub::class);

        // Test append.
        $sub_m = new Sub();
        $sub_m->setA(1);
        $arr[] = $sub_m;
        $this->assertSame(1, $arr[0]->getA());

        $this->assertEquals(1, count($arr));

        // Test set.
        $sub_m = new Sub();
        $sub_m->setA(2);
        $arr[0] = $sub_m;
        $this->assertSame(2, $arr[0]->getA());

        // Test foreach.
        $arr = new RepeatedField(GPBType::MESSAGE, Sub::class);
        for ($i = 0; $i < 3; $i++) {
          $arr[] = new Sub();
          $arr[$i]->setA($i);
        }
        $i = 0;
        foreach ($arr as $val) {
          $this->assertSame($i++, $val->getA());
        }
        $this->assertSame(3, $i);
    }

    #########################################################
    # Test offset type
    #########################################################

    public function testOffset()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;

        $arr[0] = 1;
        $this->assertSame(1, $arr[0]);
        $this->assertSame(1, count($arr));

        $arr['0'] = 2;
        $this->assertSame(2, $arr['0']);
        $this->assertSame(2, $arr[0]);
        $this->assertSame(1, count($arr));

        $arr[0.0] = 3;
        $this->assertSame(3, $arr[0.0]);
        $this->assertSame(1, count($arr));
    }

    public function testInsertRemoval()
    {
        $arr = new RepeatedField(GPBType::INT32);

        $arr[] = 0;
        $arr[] = 1;
        $arr[] = 2;
        $this->assertSame(3, count($arr));

        unset($arr[2]);
        $this->assertSame(2, count($arr));
        $this->assertSame(0, $arr[0]);
        $this->assertSame(1, $arr[1]);

        $arr[] = 3;
        $this->assertSame(3, count($arr));
        $this->assertSame(0, $arr[0]);
        $this->assertSame(1, $arr[1]);
        $this->assertSame(3, $arr[2]);
    }

    #########################################################
    # Test reference in array
    #########################################################

    public function testArrayElementIsReferenceInSetters()
    {
        // Bool elements
        $values = [true];
        array_walk($values, function (&$value) {});
        $m = new TestMessage();
        $m->setRepeatedBool($values);

        // Int32 elements
        $values = [1];
        array_walk($values, function (&$value) {});
        $m = new TestMessage();
        $m->setRepeatedInt32($values);

        // Double elements
        $values = [1.0];
        array_walk($values, function (&$value) {});
        $m = new TestMessage();
        $m->setRepeatedDouble($values);

        // String elements
        $values = ['a'];
        array_walk($values, function (&$value) {});
        $m = new TestMessage();
        $m->setRepeatedString($values);

        // Message elements
        $m = new TestMessage();
        $subs = [1, 2];
        foreach ($subs as &$sub) {
            $sub = new Sub(['a' => $sub]);
        }
        $m->setRepeatedMessage($subs);

        $this->assertTrue(true);
    }

    #########################################################
    # Test memory leak
    #########################################################

    public function testCycleLeak()
    {
        if (getenv("USE_ZEND_ALLOC") === "0") {
            // If we are disabling Zend's internal allocator (as we do for
            // Valgrind tests, for example) then memory_get_usage() will not
            // return a useful value.
            $this->markTestSkipped();
            return;
        }

        gc_collect_cycles();
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage::class);
        $arr[] = new TestMessage;
        $arr[0]->SetRepeatedRecursive($arr);

        // Clean up memory before test.
        gc_collect_cycles();
        $start = memory_get_usage();
        unset($arr);

        // Explicitly trigger garbage collection.
        gc_collect_cycles();

        $end = memory_get_usage();
        $this->assertLessThan($start, $end);
    }

    #########################################################
    # Test incorrect types
    #########################################################

    public function testAppendNull()
    {
        $this->expectException(TypeError::class);
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage::class);
        $arr[] = null;
    }

    #########################################################
    # Test equality
    #########################################################

    public function testEquality()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr2 = new RepeatedField(GPBType::INT32);

        $this->assertTrue($arr == $arr2);

        $arr[] = 0;
        $arr[] = 1;
        $arr[] = 2;

        $this->assertFalse($arr == $arr2);

        $arr2[] = 0;
        $arr2[] = 1;
        $arr2[] = 2;

        $this->assertTrue($arr == $arr2);

        // Arrays of different types always compare false.
        $this->assertFalse(new RepeatedField(GPBType::INT32) ==
                           new RepeatedField(GPBType::INT64));
        $this->assertFalse(
            new RepeatedField(GPBType::MESSAGE, TestMessage::class) ==
            new RepeatedField(GPBType::MESSAGE, Sub::class));
    }

    #########################################################
    # Test clone
    #########################################################

    public function testClone()
    {
        $arr = new RepeatedField(GPBType::MESSAGE, TestMessage::class);
        $arr[] = new TestMessage;
        $arr2 = clone $arr;
        $this->assertSame($arr[0], $arr2[0]);
    }

    #########################################################
    # Test offsetUnset
    #########################################################

    public function testOffsetUnset()
    {
        $arr = new RepeatedField(GPBType::INT32);
        $arr[] = 0;
        $arr[] = 1;
        $arr[] = 2;

        $this->assertSame(3, count($arr));
        $this->assertSame(0, $arr[0]);
        $this->assertSame(1, $arr[1]);
        $this->assertSame(2, $arr[2]);

        $arr->offsetUnset(1);
        $this->assertSame(0, $arr[0]);
        $this->assertSame(2, $arr[1]);

        $arr->offsetUnset(0);
        $this->assertSame(2, $arr[0]);

        $arr->offsetUnset(0);
        $this->assertCount(0, $arr);
    }
}
