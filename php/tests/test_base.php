<?php

use Foo\TestMessage;
use Foo\TestEnum;
use Foo\TestMessage\Sub;

class TestBase extends \PHPUnit\Framework\TestCase
{

    public function setFields(TestMessage $m)
    {
        TestUtil::setTestMessage($m);
    }

    /**
     * Polyfill for phpunit6.
     */
    static public function assertStringContains($needle, $haystack)
    {
        if (function_exists('PHPUnit\Framework\assertStringContainsString')) {
            parent::assertStringContainsString($needle, $haystack);
        } else {
            parent::assertContains($needle, $haystack);
        }
    }

    /**
     * Polyfill for phpunit6.
     */
    static public function assertFloatEquals($expected, $actual, $delta)
    {
        if (function_exists('PHPUnit\Framework\assertEqualsWithDelta')) {
            parent::assertEqualsWithDelta($expected, $actual, $delta);
        } else {
            parent::assertEquals($expected, $actual, '', $delta);
        }
    }

    public function setFields2(TestMessage $m)
    {
        TestUtil::setTestMessage2($m);
    }

    public function expectFields(TestMessage $m)
    {
        $this->assertSame(-42,  $m->getOptionalInt32());
        $this->assertSame(42,  $m->getOptionalUint32());
        $this->assertSame(-44,  $m->getOptionalSint32());
        $this->assertSame(46,   $m->getOptionalFixed32());
        $this->assertSame(-46,  $m->getOptionalSfixed32());
        $this->assertSame(1.5,  $m->getOptionalFloat());
        $this->assertSame(1.6,  $m->getOptionalDouble());
        $this->assertSame(true, $m->getOptionalBool());
        $this->assertSame('a',  $m->getOptionalString());
        $this->assertSame('bbbb',  $m->getOptionalBytes());
        $this->assertSame(TestEnum::ONE, $m->getOptionalEnum());
        $this->assertSame(33,   $m->getOptionalMessage()->getA());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('-43',  $m->getOptionalInt64());
            $this->assertSame('43',   $m->getOptionalUint64());
            $this->assertSame('-45',  $m->getOptionalSint64());
            $this->assertSame('47',   $m->getOptionalFixed64());
            $this->assertSame('-47',  $m->getOptionalSfixed64());
        } else {
            $this->assertSame(-43,  $m->getOptionalInt64());
            $this->assertSame(43,   $m->getOptionalUint64());
            $this->assertSame(-45,  $m->getOptionalSint64());
            $this->assertSame(47,   $m->getOptionalFixed64());
            $this->assertSame(-47,  $m->getOptionalSfixed64());
        }

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
        $this->assertEquals('bbbb',  $m->getRepeatedBytes()[0]);
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
        $this->assertEquals('dddd',   $m->getRepeatedBytes()[1]);
        $this->assertEquals(35,    $m->getRepeatedMessage()[1]->GetA());

        if (PHP_INT_SIZE == 4) {
            $this->assertEquals('-63', $m->getMapInt64Int64()['-63']);
            $this->assertEquals('63',  $m->getMapUint64Uint64()['63']);
            $this->assertEquals('-65', $m->getMapSint64Sint64()['-65']);
            $this->assertEquals('67',  $m->getMapFixed64Fixed64()['67']);
            $this->assertEquals('-69',  $m->getMapSfixed64Sfixed64()['-69']);
        } else {
            $this->assertEquals(-63, $m->getMapInt64Int64()[-63]);
            $this->assertEquals(63,  $m->getMapUint64Uint64()[63]);
            $this->assertEquals(-65, $m->getMapSint64Sint64()[-65]);
            $this->assertEquals(67,  $m->getMapFixed64Fixed64()[67]);
            $this->assertEquals(-69,  $m->getMapSfixed64Sfixed64()[-69]);
        }
        $this->assertEquals(-62, $m->getMapInt32Int32()[-62]);
        $this->assertEquals(62,  $m->getMapUint32Uint32()[62]);
        $this->assertEquals(-64, $m->getMapSint32Sint32()[-64]);
        $this->assertEquals(66,  $m->getMapFixed32Fixed32()[66]);
        $this->assertEquals(-68,  $m->getMapSfixed32Sfixed32()[-68]);
        $this->assertEquals(3.5, $m->getMapInt32Float()[1]);
        $this->assertEquals(3.6, $m->getMapInt32Double()[1]);
        $this->assertEquals(true , $m->getMapBoolBool()[true]);
        $this->assertEquals('e', $m->getMapStringString()['e']);
        $this->assertEquals('ffff', $m->getMapInt32Bytes()[1]);
        $this->assertEquals(TestEnum::ONE, $m->getMapInt32Enum()[1]);
        $this->assertEquals(36, $m->getMapInt32Message()[1]->GetA());
    }

    // Test message merged from setFields and setFields2.
    public function expectFieldsMerged(TestMessage $m)
    {
        $this->assertSame(-144,  $m->getOptionalSint32());
        $this->assertSame(146,   $m->getOptionalFixed32());
        $this->assertSame(-146,  $m->getOptionalSfixed32());
        $this->assertSame(11.5,  $m->getOptionalFloat());
        $this->assertSame(11.6,  $m->getOptionalDouble());
        $this->assertSame(true, $m->getOptionalBool());
        $this->assertSame('aa',  $m->getOptionalString());
        $this->assertSame('bb',  $m->getOptionalBytes());
        $this->assertSame(133,   $m->getOptionalMessage()->getA());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame('-143',  $m->getOptionalInt64());
            $this->assertSame('143',   $m->getOptionalUint64());
            $this->assertSame('-145',  $m->getOptionalSint64());
            $this->assertSame('147',   $m->getOptionalFixed64());
            $this->assertSame('-147',  $m->getOptionalSfixed64());
        } else {
            $this->assertSame(-143,  $m->getOptionalInt64());
            $this->assertSame(143,   $m->getOptionalUint64());
            $this->assertSame(-145,  $m->getOptionalSint64());
            $this->assertSame(147,   $m->getOptionalFixed64());
            $this->assertSame(-147,  $m->getOptionalSfixed64());
        }

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
        $this->assertEquals('bbbb',  $m->getRepeatedBytes()[0]);
        $this->assertEquals(TestEnum::ZERO,  $m->getRepeatedEnum()[0]);
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
        $this->assertEquals('dddd',   $m->getRepeatedBytes()[1]);
        $this->assertEquals(TestEnum::ONE,  $m->getRepeatedEnum()[1]);
        $this->assertEquals(35,    $m->getRepeatedMessage()[1]->GetA());

        $this->assertEquals(-142,  $m->getRepeatedInt32()[2]);
        $this->assertEquals(142,   $m->getRepeatedUint32()[2]);
        $this->assertEquals(-143,  $m->getRepeatedInt64()[2]);
        $this->assertEquals(143,   $m->getRepeatedUint64()[2]);
        $this->assertEquals(-144,  $m->getRepeatedSint32()[2]);
        $this->assertEquals(-145,  $m->getRepeatedSint64()[2]);
        $this->assertEquals(146,   $m->getRepeatedFixed32()[2]);
        $this->assertEquals(147,   $m->getRepeatedFixed64()[2]);
        $this->assertEquals(-146,  $m->getRepeatedSfixed32()[2]);
        $this->assertEquals(-147,  $m->getRepeatedSfixed64()[2]);
        $this->assertEquals(11.5,  $m->getRepeatedFloat()[2]);
        $this->assertEquals(11.6,  $m->getRepeatedDouble()[2]);
        $this->assertEquals(false, $m->getRepeatedBool()[2]);
        $this->assertEquals('aa',  $m->getRepeatedString()[2]);
        $this->assertEquals('bb',  $m->getRepeatedBytes()[2]);
        $this->assertEquals(TestEnum::TWO,  $m->getRepeatedEnum()[2]);
        $this->assertEquals(134,   $m->getRepeatedMessage()[2]->GetA());

        if (PHP_INT_SIZE == 4) {
            $this->assertEquals('-163', $m->getMapInt64Int64()['-63']);
            $this->assertEquals('163',  $m->getMapUint64Uint64()['63']);
            $this->assertEquals('-165', $m->getMapSint64Sint64()['-65']);
            $this->assertEquals('167',  $m->getMapFixed64Fixed64()['67']);
            $this->assertEquals('-169',  $m->getMapSfixed64Sfixed64()['-69']);
        } else {
            $this->assertEquals(-163, $m->getMapInt64Int64()[-63]);
            $this->assertEquals(163,  $m->getMapUint64Uint64()[63]);
            $this->assertEquals(-165, $m->getMapSint64Sint64()[-65]);
            $this->assertEquals(167,  $m->getMapFixed64Fixed64()[67]);
            $this->assertEquals(-169,  $m->getMapSfixed64Sfixed64()[-69]);
        }
        $this->assertEquals(-162, $m->getMapInt32Int32()[-62]);
        $this->assertEquals(162,  $m->getMapUint32Uint32()[62]);
        $this->assertEquals(-164, $m->getMapSint32Sint32()[-64]);
        $this->assertEquals(166,  $m->getMapFixed32Fixed32()[66]);
        $this->assertEquals(-168,  $m->getMapSfixed32Sfixed32()[-68]);
        $this->assertEquals(13.5, $m->getMapInt32Float()[1]);
        $this->assertEquals(13.6, $m->getMapInt32Double()[1]);
        $this->assertEquals(false , $m->getMapBoolBool()[true]);
        $this->assertEquals('ee', $m->getMapStringString()['e']);
        $this->assertEquals('ff', $m->getMapInt32Bytes()[1]);
        $this->assertEquals(TestEnum::TWO, $m->getMapInt32Enum()[1]);
        $this->assertEquals(136, $m->getMapInt32Message()[1]->GetA());

        if (PHP_INT_SIZE == 4) {
            $this->assertEquals('-163', $m->getMapInt64Int64()['-163']);
            $this->assertEquals('163',  $m->getMapUint64Uint64()['163']);
            $this->assertEquals('-165', $m->getMapSint64Sint64()['-165']);
            $this->assertEquals('167',  $m->getMapFixed64Fixed64()['167']);
            $this->assertEquals('-169',  $m->getMapSfixed64Sfixed64()['-169']);
        } else {
            $this->assertEquals(-163, $m->getMapInt64Int64()[-163]);
            $this->assertEquals(163,  $m->getMapUint64Uint64()[163]);
            $this->assertEquals(-165, $m->getMapSint64Sint64()[-165]);
            $this->assertEquals(167,  $m->getMapFixed64Fixed64()[167]);
            $this->assertEquals(-169,  $m->getMapSfixed64Sfixed64()[-169]);
        }
        $this->assertEquals(-162, $m->getMapInt32Int32()[-162]);
        $this->assertEquals(162,  $m->getMapUint32Uint32()[162]);
        $this->assertEquals(-164, $m->getMapSint32Sint32()[-164]);
        $this->assertEquals(166,  $m->getMapFixed32Fixed32()[166]);
        $this->assertEquals(-168,  $m->getMapSfixed32Sfixed32()[-168]);
        $this->assertEquals(13.5, $m->getMapInt32Float()[2]);
        $this->assertEquals(13.6, $m->getMapInt32Double()[2]);
        $this->assertEquals(false , $m->getMapBoolBool()[false]);
        $this->assertEquals('ee', $m->getMapStringString()['ee']);
        $this->assertEquals('ff', $m->getMapInt32Bytes()[2]);
        $this->assertEquals(TestEnum::TWO, $m->getMapInt32Enum()[2]);
        $this->assertEquals(136, $m->getMapInt32Message()[2]->GetA());
    }

    public function expectEmptyFields(TestMessage $m)
    {
        $this->assertSame(0,   $m->getOptionalInt32());
        $this->assertSame(0,   $m->getOptionalUint32());
        $this->assertSame(0,   $m->getOptionalSint32());
        $this->assertSame(0,   $m->getOptionalFixed32());
        $this->assertSame(0,   $m->getOptionalSfixed32());
        $this->assertSame(0.0, $m->getOptionalFloat());
        $this->assertSame(0.0, $m->getOptionalDouble());
        $this->assertSame(false, $m->getOptionalBool());
        $this->assertSame('',  $m->getOptionalString());
        $this->assertSame('',  $m->getOptionalBytes());
        $this->assertSame(0, $m->getOptionalEnum());
        $this->assertNull($m->getOptionalMessage());
        $this->assertNull($m->getOptionalIncludedMessage());
        $this->assertNull($m->getRecursive());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame("0", $m->getOptionalInt64());
            $this->assertSame("0", $m->getOptionalUint64());
            $this->assertSame("0", $m->getOptionalSint64());
            $this->assertSame("0", $m->getOptionalFixed64());
            $this->assertSame("0", $m->getOptionalSfixed64());
        } else {
            $this->assertSame(0, $m->getOptionalInt64());
            $this->assertSame(0, $m->getOptionalUint64());
            $this->assertSame(0, $m->getOptionalSint64());
            $this->assertSame(0, $m->getOptionalFixed64());
            $this->assertSame(0, $m->getOptionalSfixed64());
        }

        $this->assertEquals(0, count($m->getRepeatedInt32()));
        $this->assertEquals(0, count($m->getRepeatedUint32()));
        $this->assertEquals(0, count($m->getRepeatedInt64()));
        $this->assertEquals(0, count($m->getRepeatedUint64()));
        $this->assertEquals(0, count($m->getRepeatedSint32()));
        $this->assertEquals(0, count($m->getRepeatedSint64()));
        $this->assertEquals(0, count($m->getRepeatedFixed32()));
        $this->assertEquals(0, count($m->getRepeatedFixed64()));
        $this->assertEquals(0, count($m->getRepeatedSfixed32()));
        $this->assertEquals(0, count($m->getRepeatedSfixed64()));
        $this->assertEquals(0, count($m->getRepeatedFloat()));
        $this->assertEquals(0, count($m->getRepeatedDouble()));
        $this->assertEquals(0, count($m->getRepeatedBool()));
        $this->assertEquals(0, count($m->getRepeatedString()));
        $this->assertEquals(0, count($m->getRepeatedBytes()));
        $this->assertEquals(0, count($m->getRepeatedEnum()));
        $this->assertEquals(0, count($m->getRepeatedMessage()));
        $this->assertEquals(0, count($m->getRepeatedRecursive()));

        $this->assertSame("", $m->getMyOneof());
        $this->assertSame(0,   $m->getOneofInt32());
        $this->assertSame(0,   $m->getOneofUint32());
        $this->assertSame(0,   $m->getOneofSint32());
        $this->assertSame(0,   $m->getOneofFixed32());
        $this->assertSame(0,   $m->getOneofSfixed32());
        $this->assertSame(0.0, $m->getOneofFloat());
        $this->assertSame(0.0, $m->getOneofDouble());
        $this->assertSame(false, $m->getOneofBool());
        $this->assertSame('',  $m->getOneofString());
        $this->assertSame('',  $m->getOneofBytes());
        $this->assertSame(0, $m->getOneofEnum());
        $this->assertNull($m->getOptionalMessage());
        if (PHP_INT_SIZE == 4) {
            $this->assertSame("0", $m->getOneofInt64());
            $this->assertSame("0", $m->getOneofUint64());
            $this->assertSame("0", $m->getOneofSint64());
            $this->assertSame("0", $m->getOneofFixed64());
            $this->assertSame("0", $m->getOneofSfixed64());
        } else {
            $this->assertSame(0, $m->getOneofInt64());
            $this->assertSame(0, $m->getOneofUint64());
            $this->assertSame(0, $m->getOneofSint64());
            $this->assertSame(0, $m->getOneofFixed64());
            $this->assertSame(0, $m->getOneofSfixed64());
        }

        $this->assertEquals(0, count($m->getMapInt64Int64()));
        $this->assertEquals(0, count($m->getMapUint64Uint64()));
        $this->assertEquals(0, count($m->getMapSint64Sint64()));
        $this->assertEquals(0, count($m->getMapFixed64Fixed64()));
        $this->assertEquals(0, count($m->getMapInt32Int32()));
        $this->assertEquals(0, count($m->getMapUint32Uint32()));
        $this->assertEquals(0, count($m->getMapSint32Sint32()));
        $this->assertEquals(0, count($m->getMapFixed32Fixed32()));
        $this->assertEquals(0, count($m->getMapSfixed32Sfixed32()));
        $this->assertEquals(0, count($m->getMapSfixed64Sfixed64()));
        $this->assertEquals(0, count($m->getMapInt32Float()));
        $this->assertEquals(0, count($m->getMapInt32Double()));
        $this->assertEquals(0, count($m->getMapBoolBool()));
        $this->assertEquals(0, count($m->getMapStringString()));
        $this->assertEquals(0, count($m->getMapInt32Bytes()));
        $this->assertEquals(0, count($m->getMapInt32Enum()));
        $this->assertEquals(0, count($m->getMapInt32Message()));
        $this->assertEquals(0, count($m->getMapRecursive()));
    }

  // This test is to avoid the warning of no test by php unit.
  public function testNone()
  {
      $this->assertTrue(true);
  }
}
