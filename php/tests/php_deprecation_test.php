<?php

require_once('test_base.php');
require_once('test_util.php');

use Foo\TestMessage\Sub;
use Foo\TestMessage_Sub;
use Symfony\Bridge\PhpUnit\ExpectDeprecationTrait;

/**
 * Please note, this test is only intended to be run without the protobuf C
 * extension.
 */
class DeprecationTest extends TestBase
{
    use ExpectDeprecationTrait;

    public function setUp()
    {
        if (extension_loaded('protobuf')) {
            $this->markTestSkipped();
        }
    }

    /**
     * @group legacy
     */
    public function testDeprecatedSubMessage()
    {
        $this->expectDeprecation(sprintf(
            '%s is deprecated and will be removed in the next major release. Use %s instead',
            TestMessage_Sub::class,
            Sub::class,
        ));

        $sub_m = new TestMessage_Sub();
        $this->assertInstanceOf(Sub::class, $sub_m);
    }
}
