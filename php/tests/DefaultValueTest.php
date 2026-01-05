<?php

require_once('test_base.php');

use Default_value\TestDefaultValue;
use Default_value\TestEnum;

class DefaultValueTest extends TestBase
{
    public function testDefaultValues()
    {
        $message = new TestDefaultValue();

        $this->assertSame(1, $message->getOptionalInt32());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('2', $message->getOptionalInt64());
        } else {
            $this->assertSame(2, $message->getOptionalInt64());
        }
        $this->assertSame(3, $message->getOptionalUint32());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('4', $message->getOptionalUint64());
        } else {
            $this->assertSame(4, $message->getOptionalUint64());
        }
        $this->assertSame(5.5, $message->getOptionalFloat());
        $this->assertSame(6.6, $message->getOptionalDouble());
        $this->assertSame(true, $message->getOptionalBool());
        $this->assertSame("hello", $message->getOptionalString());
        $this->assertSame("world", $message->getOptionalBytes());
        $this->assertSame(TestEnum::HELLO, $message->getOptionalEnum());
    }
}
