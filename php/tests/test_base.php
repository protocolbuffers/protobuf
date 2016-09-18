<?php

use Foo\TestMessage;
use Foo\TestMessage_Sub;

class TestBase extends PHPUnit_Framework_TestCase
{

    public function setFields(TestMessage $m)
    {
        TestUtil::setTestMessage($m);
    }

    public function expectFields(TestMessage $m)
    {
        $this->assertSame(-42,  $m->getOptionalInt32());
        $this->assertSame(42,   $m->getOptionalUint32());
        $this->assertSame(-43,  $m->getOptionalInt64());
        $this->assertSame(43,   $m->getOptionalUint64());
        $this->assertSame(-44,  $m->getOptionalSint32());
        $this->assertSame(-45,  $m->getOptionalSint64());
        $this->assertSame(46,   $m->getOptionalFixed32());
        $this->assertSame(47,   $m->getOptionalFixed64());
        $this->assertSame(-46,  $m->getOptionalSfixed32());
        $this->assertSame(-47,  $m->getOptionalSfixed64());
        $this->assertSame(1.5,  $m->getOptionalFloat());
        $this->assertSame(1.6,  $m->getOptionalDouble());
        $this->assertSame(true, $m->getOptionalBool());
        $this->assertSame('a',  $m->getOptionalString());
        $this->assertSame('b',  $m->getOptionalBytes());
        $this->assertSame(33,   $m->getOptionalMessage()->getA());

        $this->assertEquals(-42,  $m->getRepeatedInt32()[0]);
        $this->assertEquals(42,   $m->getRepeatedUint32()[0]);
        $this->assertEquals(-43,  $m->getRepeatedInt64()[0]);
        $this->assertEquals(43,   $m->getRepeatedUint64()[0]);
        $this->assertEquals(-44,  $m->getRepeatedSint32()[0]);
        $this->assertEquals(-45,  $m->getRepeatedSint64()[0]);
        $this->assertEquals(46,   $m->getRepeatedFixed32()[0]);
        $this->assertEquals(47,   $m->getRepeatedFixed64()[0]);
        $this->assertEquals(-46,  $m->getRepeatedSfixed32()[0]);
        $this->assertEquals(-47,  $m->getRepeatedSfixed64()[0]);
        $this->assertEquals(1.5,  $m->getRepeatedFloat()[0]);
        $this->assertEquals(1.6,  $m->getRepeatedDouble()[0]);
        $this->assertEquals(true, $m->getRepeatedBool()[0]);
        $this->assertEquals('a',  $m->getRepeatedString()[0]);
        $this->assertEquals('b',  $m->getRepeatedBytes()[0]);
        $this->assertEquals(34,   $m->getRepeatedMessage()[0]->GetA());

        $this->assertEquals(-52,   $m->getRepeatedInt32()[1]);
        $this->assertEquals(52,    $m->getRepeatedUint32()[1]);
        $this->assertEquals(-53,   $m->getRepeatedInt64()[1]);
        $this->assertEquals(53,    $m->getRepeatedUint64()[1]);
        $this->assertEquals(-54,   $m->getRepeatedSint32()[1]);
        $this->assertEquals(-55,   $m->getRepeatedSint64()[1]);
        $this->assertEquals(56,    $m->getRepeatedFixed32()[1]);
        $this->assertEquals(57,    $m->getRepeatedFixed64()[1]);
        $this->assertEquals(-56,   $m->getRepeatedSfixed32()[1]);
        $this->assertEquals(-57,   $m->getRepeatedSfixed64()[1]);
        $this->assertEquals(2.5,   $m->getRepeatedFloat()[1]);
        $this->assertEquals(2.6,   $m->getRepeatedDouble()[1]);
        $this->assertEquals(false, $m->getRepeatedBool()[1]);
        $this->assertEquals('c',   $m->getRepeatedString()[1]);
        $this->assertEquals('d',   $m->getRepeatedBytes()[1]);
        $this->assertEquals(35,    $m->getRepeatedMessage()[1]->GetA());
    }

    public function expectEmptyFields(TestMessage $m)
    {
        $this->assertSame(0,   $m->getOptionalInt32());
        $this->assertSame(0,   $m->getOptionalUint32());
        $this->assertSame(0,   $m->getOptionalInt64());
        $this->assertSame(0,   $m->getOptionalUint64());
        $this->assertSame(0,   $m->getOptionalSint32());
        $this->assertSame(0,   $m->getOptionalSint64());
        $this->assertSame(0,   $m->getOptionalFixed32());
        $this->assertSame(0,   $m->getOptionalFixed64());
        $this->assertSame(0,   $m->getOptionalSfixed32());
        $this->assertSame(0,   $m->getOptionalSfixed64());
        $this->assertSame(0.0, $m->getOptionalFloat());
        $this->assertSame(0.0, $m->getOptionalDouble());
        $this->assertSame(false, $m->getOptionalBool());
        $this->assertSame('',  $m->getOptionalString());
        $this->assertSame('',  $m->getOptionalBytes());
        $this->assertNull($m->getOptionalMessage());
    }

  // This test is to avoid the warning of no test by php unit.
  public function testNone()
  {
  }
}
