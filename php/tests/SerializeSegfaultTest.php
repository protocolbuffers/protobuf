<?php
require_once('test_base.php');
require_once('test_util.php');

use Foo\TestMessage;

class SerializeSegfaultTest extends TestBase {

    public function testSerialize() {
        $msg = new TestMessage(['optional_string' => "Hello, World!"]);
        if (extension_loaded('protobuf')) {
            $this->expectException(\Exception::class);
            $this->expectExceptionMessage("Serialization of 'Google\Protobuf\Internal\Message' is not allowed");
            serialize($msg);
        } else {
            $this->markTestSkipped('Native serialization is allowed in pure-PHP mode.');
        }
        serialize($msg);
    }

}
