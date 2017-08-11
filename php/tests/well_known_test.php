<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\GPBEmpty;
use Google\Protobuf\Any;

use Foo\TestMessage;

class NotMessage {}

class WellKnownTest extends TestBase {

    public function testNone()
    {
        $msg = new GPBEmpty();
    }

    public function testImportDescriptorProto()
    {
        $msg = new TestImportDescriptorProto();
    }

    public function testAny()
    {
        // Create embed message
        $embed = new TestMessage();
        $this->setFields($embed);
        $data = $embed->serializeToString();

        // Set any via normal setter.
        $any = new Any();

        $this->assertSame(
            $any, $any->setTypeUrl("type.googleapis.com/foo.TestMessage"));
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $any->getTypeUrl());

        $this->assertSame($any, $any->setValue($data));
        $this->assertSame($data, $any->getValue());

        // Test unpack.
        $msg = $any->unpack();
        $this->assertTrue($msg instanceof TestMessage);
        $this->expectFields($msg);

        // Test pack.
        $any = new Any();
        $any->pack($embed);
        $this->assertSame($data, $any->getValue());
        $this->assertSame("type.googleapis.com/foo.TestMessage", $any->getTypeUrl());

        // Test is.
        $this->assertTrue($any->is(TestMessage::class));
        $this->assertFalse($any->is(Any::class));
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackInvalidTypeUrl()
    {
        $any = new Any();
        $any->setTypeUrl("invalid");
        $any->unpack();
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackMessageNotAdded()
    {
        $any = new Any();
        $any->setTypeUrl("type.googleapis.com/MessageNotAdded");
        $any->unpack();
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackDecodeError()
    {
        $any = new Any();
        $any->setTypeUrl("type.googleapis.com/foo.TestMessage");
        $any->setValue("abc");
        $any->unpack();
    }
}
