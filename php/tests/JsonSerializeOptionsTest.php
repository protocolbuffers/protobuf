<?php

use Google\Protobuf\JsonSerializeOptions;

require_once('test_base.php');

class JsonSerializeOptionsTest extends TestBase
{
    public function testPreserveProtoFieldNames()
    {
        $sut = new TestPreserveProtoFieldNames();
        $sut->setShort('long name');
        $sut->setLongName('long name');
        $sut->setStrangeCase('some strange case');
        $sut->setWithJsonName('with json name');
        $data = json_decode(
            $sut->serializeToJsonString([JsonSerializeOptions::PRESERVE_PROTO_FIELD_NAMES => true]),
            true
        );

        self::assertArrayHasKey('short', $data);
        self::assertArrayHasKey('long_name', $data);
        self::assertArrayHasKey('Strange_case', $data);
        self::assertArrayHasKey('json_name', $data);

        $data = json_decode($sut->serializeToJsonString(), true);
        self::assertArrayHasKey('short', $data);
        self::assertArrayHasKey('longName', $data);
        self::assertArrayHasKey('StrangeCase', $data);
        self::assertArrayHasKey('json_name', $data);
    }

    public function testEmitsDefaults()
    {
        $sut = new TestEmitsDefaults();
        $data = json_decode($sut->serializeToJsonString(), true);

        self::assertEquals([], $data);

        $data = json_decode($sut->serializeToJsonString([JsonSerializeOptions::EMIT_DEFAULTS => true]), true);
        self::assertEquals(
            [
                'valueString' => '',
                'valueInt32' => 0,
                'valueInt64' => '0',
                'valueBool' => false,
                'valueDouble' => 0,
                'valueFloat' => 0,
                'valueRepeated' => []
            ],
            $data
        );
    }
}
