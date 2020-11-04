<?php

require_once('test_util.php');

use Foo\TestMessage;

class HasOneofTest extends \PHPUnit\Framework\TestCase {

    #########################################################
    # Test hasOneof<Field> methods exists and working
    #########################################################

    public function testHasOneof() {
        $m = new TestMessage();
        $this->assertFalse($m->hasOneofInt32());
        $m->setOneofInt32(42);
        $this->assertTrue($m->hasOneofInt32());
        $m->setOneofString("bar");
        $this->assertFalse($m->hasOneofInt32());
        $this->assertTrue($m->hasOneofString());
        $m->clear();
        $this->assertFalse($m->hasOneofInt32());
        $this->assertFalse($m->hasOneofString());
    }

}
