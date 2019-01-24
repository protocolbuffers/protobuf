<?php

require_once 'test_base.php';

use Google\Protobuf\Internal\GPBUtil;

final class GPBUtilTest extends TestBase
{
    /**
     * @dataProvider dataProviderCheckUint32
     */
    public function testCheckUint32($expectedValue, $inputValue)
    {
        GPBUtil::checkUint32($inputValue);

        self::assertSame($expectedValue, $inputValue);
    }

    public function dataProviderCheckUint32()
    {
        yield 'min uint32 value' => [0, 0];
        yield 'max uint32 value' => [4294967295, 4294967295];
        yield 'uint32 overflow' => [0, 4294967296];
    }
}
