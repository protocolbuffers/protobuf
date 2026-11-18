<?php

require_once('test_base.php');
require_once('test_util.php');

use Foo\TestEnum;
use Foo\TestMessage;
use Foo\TestMessage\Sub;
use Foo\TestStringValue;
use Google\Protobuf\Any;
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\MapField;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Timestamp;

class OomTest extends TestBase
{
    private $dateTime;

    private function manyAllocsScenario()
    {
        $m = new TestMessage();
        $m->setOptionalInt32(123);
        $m->setOptionalInt64(456);
        $m->setOptionalUint32(789);
        $m->setOptionalUint64(101112);
        $m->setOptionalBool(true);
        $m->setOptionalFloat(1.5);
        $m->setOptionalDouble(2.5);
        $m->setOptionalString('hello');
        $m->setOptionalBytes("world\x00escape");
        $m->setOptionalEnum(TestEnum::ONE);

        $sub = new Sub();
        $sub->setA(42);
        $m->setOptionalMessage($sub);

        // Oneof fields (writeOneof)
        $m->setOneofInt32(11);
        $m->setOneofString('oneof_str');
        $oneofSub = new Sub();
        $oneofSub->setA(99);
        $m->setOneofMessage($oneofSub);

        // Repeated fields via Message_get (upb_Message_Mutable) and offsetSet/append
        $repInt = $m->getRepeatedInt32();
        for ($i = 0; $i < 32; $i++) {
            $repInt[] = $i;
        }
        $m->getRepeatedString()[] = 'foo';
        $m->getRepeatedBytes()[] = 'bar';
        $repSub = new Sub();
        $repSub->setA(43);
        $m->getRepeatedMessage()[] = $repSub;

        // Standalone RepeatedField (__construct, append, offsetSet, clone)
        $standaloneArr = new RepeatedField(GPBType::STRING);
        $standaloneArr->append('item1');
        $standaloneArr[] = 'item2';
        $clonedArr = clone $standaloneArr;
        $m->setRepeatedString($clonedArr);

        // Map fields via Message_get (upb_Message_Mutable) and offsetSet
        $mapInt = $m->getMapInt32Int32();
        for ($i = 0; $i < 16; $i++) {
            $mapInt[$i] = $i * 10;
        }
        $m->getMapStringString()['k'] = 'v';
        $mapSub = new Sub();
        $mapSub->setA(44);
        $m->getMapInt32Message()[1] = $mapSub;

        // Standalone MapField (__construct, offsetSet, clone)
        $standaloneMap = new MapField(GPBType::STRING, GPBType::STRING);
        $standaloneMap['a'] = 'alpha';
        $standaloneMap['b'] = 'beta';
        $clonedMap = clone $standaloneMap;
        $m->setMapStringString($clonedMap);

        // Array initialization (Message_InitFromPhp, RepeatedField_GetUpbArray,
        // MapField_GetUpbMap, Convert_PhpToUpbAutoWrap)
        $initSub = new Sub(['a' => 55]);
        $mFromArr = new TestMessage([
            'optional_int32' => -42,
            'optional_string' => 'init_str',
            'optional_message' => $initSub,
            'repeated_int32' => [1, 2, 3, 4],
            'map_int32_int32' => [1 => 10, 2 => 20],
        ]);

        // Wrapper value setters and auto-wrap (writeWrapperValue, Convert_PhpToUpbAutoWrap)
        $wrapperMsg = new TestStringValue([
            'field' => 'autowrapped',
            'repeated_field' => ['r1', 'r2'],
            'map_field' => [1 => 'm1'],
        ]);
        @$wrapperMsg->setFieldUnwrapped('unwrapped_str');

        // Clone message (Message_clone_obj -> upb_Message_ShallowClone)
        $clonedMsg = clone $m;

        // Serialize, decode, and merge
        $serialized = $m->serializeToString();
        $decoded = new TestMessage();
        $decoded->mergeFromString($serialized);

        $merged = new TestMessage();
        $merged->mergeFrom($mFromArr);

        // Well-known types: Any (pack, unpack) and Timestamp (fromDateTime)
        $any = new Any();
        $any->pack($sub);
        $unpacked = $any->unpack();

        $ts = new Timestamp();
        $ts->fromDateTime($this->dateTime);
    }

    public function testOom()
    {
        if (!extension_loaded('protobuf')) {
            $this->markTestSkipped('Requires C extension');
        }
        if (!\Google\Protobuf\Internal\allocation_count_is_available()) {
            $this->markTestSkipped('Requires Debug-only allocation_count API');
        }

        date_default_timezone_set('UTC');
        $this->dateTime = new \DateTime('2011-01-01T15:03:01.012345UTC');

        // Warm up descriptor/name caches so we get a consistent allocation count.
        $this->manyAllocsScenario();

        \Google\Protobuf\Internal\allocation_count_reset();
        $this->manyAllocsScenario();
        $total = \Google\Protobuf\Internal\allocation_count_get();
        $this->assertGreaterThan(0, $total);

        try {
            for ($i = 0; $i < $total; $i++) {
                \Google\Protobuf\Internal\allocation_count_reset();
                \Google\Protobuf\Internal\allocation_count_fail_on($i);
                try {
                    $this->manyAllocsScenario();
                } catch (\Exception $e) {
                    $this->assertSame('Out of memory', $e->getMessage());
                    continue;
                }
                if (\Google\Protobuf\Internal\allocation_count_get() > $i) {
                    $this->fail(sprintf(
                        'Out of memory exception was expected at allocation %d, but completed with %d allocations.',
                        $i,
                        \Google\Protobuf\Internal\allocation_count_get()
                    ));
                }
            }
        } finally {
            \Google\Protobuf\Internal\allocation_count_reset();
        }
    }
}
