<?php

require_once 'test_base.php';

use Google\Protobuf\Internal\GPBUtil;

final class GPBUtilTest extends TestBase
{
    /**
     * @dataProvider providerCheckUint32
     */
    public function testCheckUint32($expectedValue, $inputValue)
    {
        GPBUtil::checkUint32($inputValue);

        self::assertSame($expectedValue, $inputValue);
    }

    public function providerCheckUint32()
    {
        yield 'negative value' => [4294967295, -1];
        yield '- max uint32 value+1' => [0, -4294967296];
        yield 'min uint32 value' => [0, 0];
        yield 'max uint32 value' => [4294967295, 4294967295];
        yield 'max uint32 value+1' => [0, 4294967296];
        yield 'uint32 overflow' => [0, 4294967296];
    }

    /**
     * @dataProvider providerInvalidValueGivenToCheckUint32
     */
    public function testInvalidValueGivenToCheckUint32($inputValue)
    {
        $this->expectException(InvalidArgumentException::class);

        GPBUtil::checkUint32($inputValue);
    }

    public function providerInvalidValueGivenToCheckUint32()
    {
        yield 'non-numeric string' => ['Lorem ipsum'];
        yield 'boolean' => [false];
    }
}
