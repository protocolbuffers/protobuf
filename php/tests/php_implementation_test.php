<?php

require_once('test.pb.php');
require_once('test_base.php');
require_once('test_util.php');

use Foo\TestMessage;
use Foo\TestMessage_Sub;
use Foo\TestPackedMessage;
use Google\Protobuf\Internal\InputStream;
use Google\Protobuf\Internal\FileDescriptorSet;
use Google\Protobuf\Internal\GPBUtil;
use Google\Protobuf\Internal\Int64;
use Google\Protobuf\Internal\Uint64;
use Google\Protobuf\Internal\GPBLabel;
use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\GPBWire;
use Google\Protobuf\Internal\OutputStream;
use Google\Protobuf\Internal\RepeatedField;

class ImplementationTest extends TestBase
{

    public function testReadInt32()
    {
        $value = null;

        // Positive number.
        $input = new InputStream(hex2bin("01"));
        GPBWire::readInt32($input, $value);
        $this->assertSame(1, $value);

        // Negative number.
        $input = new InputStream(hex2bin("ffffffff0f"));
        GPBWire::readInt32($input, $value);
        $this->assertSame(-1, $value);

        // Discard overflow bits.
        $input = new InputStream(hex2bin("ffffffff7f"));
        GPBWire::readInt32($input, $value);
        $this->assertSame(-1, $value);
    }

    public function testReadUint32()
    {
        $value = null;

        // Positive number.
        $input = new InputStream(hex2bin("01"));
        GPBWire::readUint32($input, $value);
        $this->assertSame(1, $value);

        // Max uint32.
        $input = new InputStream(hex2bin("ffffffff0f"));
        GPBWire::readUint32($input, $value);
        $this->assertSame(-1, $value);

        // Discard overflow bits.
        $input = new InputStream(hex2bin("ffffffff7f"));
        GPBWire::readUint32($input, $value);
        $this->assertSame(-1, $value);
    }

    public function testReadInt64()
    {
        $value = null;

        // Positive number.
        $input = new InputStream(hex2bin("01"));
        GPBWire::readInt64($input, $value);
        $this->assertSame(1, $value->toInteger());

        // Negative number.
        $input = new InputStream(hex2bin("ffffffffffffffffff01"));
        GPBWire::readInt64($input, $value);
        $this->assertSame(-1, $value->toInteger());

        // Discard overflow bits.
        $input = new InputStream(hex2bin("ffffffffffffffffff0f"));
        GPBWire::readInt64($input, $value);
        $this->assertSame(-1, $value->toInteger());
    }

    public function testReadUint64()
    {
        $value = null;

        // Positive number.
        $input = new InputStream(hex2bin("01"));
        GPBWire::readUint64($input, $value);
        $this->assertSame(1, $value->toInteger());

        // Negative number.
        $input = new InputStream(hex2bin("FFFFFFFFFFFFFFFFFF01"));
        GPBWire::readUint64($input, $value);
        $this->assertSame(-1, $value->toInteger());

        // Discard overflow bits.
        $input = new InputStream(hex2bin("FFFFFFFFFFFFFFFFFF0F"));
        GPBWire::readUint64($input, $value);
        $this->assertSame(-1, $value->toInteger());
    }

    public function testReadSint32()
    {
        $value = null;

        $input = new InputStream(hex2bin("00"));
        GPBWire::readSint32($input, $value);
        $this->assertSame(0, $value);

        $input = new InputStream(hex2bin("01"));
        GPBWire::readSint32($input, $value);
        $this->assertSame(-1, $value);

        $input = new InputStream(hex2bin("02"));
        GPBWire::readSint32($input, $value);
        $this->assertSame(1, $value);
    }

    public function testReadSint64()
    {
        $value = null;

        $input = new InputStream(hex2bin("00"));
        GPBWire::readSint64($input, $value);
        $this->assertEquals(GPBUtil::Int64(0), $value);

        $input = new InputStream(hex2bin("01"));
        GPBWire::readSint64($input, $value);
        $this->assertEquals(GPBUtil::Int64(-1), $value);

        $input = new InputStream(hex2bin("02"));
        GPBWire::readSint64($input, $value);
        $this->assertEquals(GPBUtil::Int64(1), $value);
    }

    public function testReadFixed32()
    {
        $value = null;
        $input = new InputStream(hex2bin("12345678"));
        GPBWire::readFixed32($input, $value);
        $this->assertSame(0x78563412, $value);
    }

    public function testReadFixed64()
    {
        $value = null;
        $input = new InputStream(hex2bin("1234567812345678"));
        GPBWire::readFixed64($input, $value);
        $this->assertEquals(Uint64::newValue(0x78563412, 0x78563412), $value);
    }

    public function testReadSfixed32()
    {
        $value = null;
        $input = new InputStream(hex2bin("12345678"));
        GPBWire::readSfixed32($input, $value);
        $this->assertSame(0x78563412, $value);
    }

    public function testReadFloat()
    {
        $value = null;
        $input = new InputStream(hex2bin("0000803F"));
        GPBWire::readFloat($input, $value);
        $this->assertSame(1.0, $value);
    }

    public function testReadBool()
    {
        $value = null;

        $input = new InputStream(hex2bin("00"));
        GPBWire::readBool($input, $value);
        $this->assertSame(false, $value);

        $input = new InputStream(hex2bin("01"));
        GPBWire::readBool($input, $value);
        $this->assertSame(true, $value);
    }

    public function testReadDouble()
    {
        $value = null;
        $input = new InputStream(hex2bin("000000000000F03F"));
        GPBWire::readDouble($input, $value);
        $this->assertSame(1.0, $value);
    }

    public function testReadSfixed64()
    {
        $value = null;
        $input = new InputStream(hex2bin("1234567812345678"));
        GPBWire::readSfixed64($input, $value);
        $this->assertEquals(Int64::newValue(0x78563412, 0x78563412), $value);
    }

    public function testZigZagEncodeDecode()
    {
        $this->assertSame(0, GPBWire::zigZagEncode32(0));
        $this->assertSame(1, GPBWire::zigZagEncode32(-1));
        $this->assertSame(2, GPBWire::zigZagEncode32(1));
        $this->assertSame(3, GPBWire::zigZagEncode32(-2));
        $this->assertSame(0x7FFFFFFE, GPBWire::zigZagEncode32(0x3FFFFFFF));
        $this->assertSame(0x7FFFFFFF, GPBWire::zigZagEncode32(0xC0000000));
        $this->assertSame(-2, GPBWire::zigZagEncode32(0x7FFFFFFF));
        $this->assertSame(-1, GPBWire::zigZagEncode32(0x80000000));

        $this->assertSame(0,  GPBWire::zigZagDecode32(0));
        $this->assertSame(-1, GPBWire::zigZagDecode32(1));
        $this->assertSame(1,  GPBWire::zigZagDecode32(2));
        $this->assertSame(-2, GPBWire::zigZagDecode32(3));
        $this->assertSame(0x3FFFFFFF,  GPBWire::zigZagDecode32(0x7FFFFFFE));
        $this->assertSame(-1073741824, GPBWire::zigZagDecode32(0x7FFFFFFF));
        $this->assertSame(0x7FFFFFFF,  GPBWire::zigZagDecode32(0xFFFFFFFE));
        $this->assertSame(-2147483648, GPBWire::zigZagDecode32(0xFFFFFFFF));

        $this->assertEquals(GPBUtil::Uint64(0),
                        GPBWire::zigZagEncode64(GPBUtil::Int64(0)));
        $this->assertEquals(GPBUtil::Uint64(1),
                        GPBWire::zigZagEncode64(GPBUtil::Int64(-1)));
        $this->assertEquals(GPBUtil::Uint64(2),
                        GPBWire::zigZagEncode64(GPBUtil::Int64(1)));
        $this->assertEquals(GPBUtil::Uint64(3),
                        GPBWire::zigZagEncode64(GPBUtil::Int64(-2)));
        $this->assertEquals(
        GPBUtil::Uint64(0x000000007FFFFFFE),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0x000000003FFFFFFF)));
        $this->assertEquals(
        GPBUtil::Uint64(0x000000007FFFFFFF),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0xFFFFFFFFC0000000)));
        $this->assertEquals(
        GPBUtil::Uint64(0x00000000FFFFFFFE),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0x000000007FFFFFFF)));
        $this->assertEquals(
        GPBUtil::Uint64(0x00000000FFFFFFFF),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0xFFFFFFFF80000000)));
        $this->assertEquals(
        Uint64::newValue(4294967295, 4294967294),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0x7FFFFFFFFFFFFFFF)));
        $this->assertEquals(
        Uint64::newValue(4294967295, 4294967295),
        GPBWire::zigZagEncode64(GPBUtil::Int64(0x8000000000000000)));

        $this->assertEquals(GPBUtil::Int64(0),
                        GPBWire::zigZagDecode64(GPBUtil::Uint64(0)));
        $this->assertEquals(GPBUtil::Int64(-1),
                        GPBWire::zigZagDecode64(GPBUtil::Uint64(1)));
        $this->assertEquals(GPBUtil::Int64(1),
                        GPBWire::zigZagDecode64(GPBUtil::Uint64(2)));
        $this->assertEquals(GPBUtil::Int64(-2),
                        GPBWire::zigZagDecode64(GPBUtil::Uint64(3)));

        // Round trip
        $this->assertSame(0, GPBWire::zigZagDecode32(GPBWire::zigZagEncode32(0)));
        $this->assertSame(1, GPBWire::zigZagDecode32(GPBWire::zigZagEncode32(1)));
        $this->assertSame(-1, GPBWire::zigZagDecode32(GPBWire::zigZagEncode32(-1)));
        $this->assertSame(14927,
                      GPBWire::zigZagDecode32(GPBWire::zigZagEncode32(14927)));
        $this->assertSame(-3612,
                      GPBWire::zigZagDecode32(GPBWire::zigZagEncode32(-3612)));
    }

    public function testDecode()
    {
        $m = new TestMessage();
        $m->decode(TestUtil::getGoldenTestMessage());
        TestUtil::assertTestMessage($m);
    }

    public function testDescriptorDecode()
    {
        $file_desc_set = new FileDescriptorSet();
        $file_desc_set->decode(hex2bin(
            "0a3b0a12746573745f696e636c7564652e70726f746f120362617222180a" .
            "0b54657374496e636c75646512090a0161180120012805620670726f746f33"));

        $this->assertSame(1, sizeof($file_desc_set->getFile()));

        $file_desc = $file_desc_set->getFile()[0];
        $this->assertSame("test_include.proto", $file_desc->getName());
        $this->assertSame("bar", $file_desc->getPackage());
        $this->assertSame(0, sizeof($file_desc->getDependency()));
        $this->assertSame(1, sizeof($file_desc->getMessageType()));
        $this->assertSame(0, sizeof($file_desc->getEnumType()));
        $this->assertSame("proto3", $file_desc->getSyntax());

        $desc = $file_desc->getMessageType()[0];
        $this->assertSame("TestInclude", $desc->getName());
        $this->assertSame(1, sizeof($desc->getField()));
        $this->assertSame(0, sizeof($desc->getNestedType()));
        $this->assertSame(0, sizeof($desc->getEnumType()));
        $this->assertSame(0, sizeof($desc->getOneofDecl()));

        $field = $desc->getField()[0];
        $this->assertSame("a", $field->getName());
        $this->assertSame(1, $field->getNumber());
        $this->assertSame(GPBLabel::OPTIONAL, $field->getLabel());
        $this->assertSame(GPBType::INT32, $field->getType());
    }

    public function testReadVarint64()
    {
        $var = 0;

        // Empty buffer.
        $input = new InputStream(hex2bin(''));
        $this->assertFalse($input->readVarint64($var));

        // The largest varint is 10 bytes long.
        $input = new InputStream(hex2bin('8080808080808080808001'));
        $this->assertFalse($input->readVarint64($var));

        // Corrupted varint.
        $input = new InputStream(hex2bin('808080'));
        $this->assertFalse($input->readVarint64($var));

        // Normal case.
        $input = new InputStream(hex2bin('808001'));
        $this->assertTrue($input->readVarint64($var));
        $this->assertSame(16384, $var->toInteger());
        $this->assertFalse($input->readVarint64($var));

        // Read two varint.
        $input = new InputStream(hex2bin('808001808002'));
        $this->assertTrue($input->readVarint64($var));
        $this->assertSame(16384, $var->toInteger());
        $this->assertTrue($input->readVarint64($var));
        $this->assertSame(32768, $var->toInteger());
        $this->assertFalse($input->readVarint64($var));
    }

    public function testReadVarint32()
    {
        $var = 0;

        // Empty buffer.
        $input = new InputStream(hex2bin(''));
        $this->assertFalse($input->readVarint32($var));

        // The largest varint is 10 bytes long.
        $input = new InputStream(hex2bin('8080808080808080808001'));
        $this->assertFalse($input->readVarint32($var));

        // Corrupted varint.
        $input = new InputStream(hex2bin('808080'));
        $this->assertFalse($input->readVarint32($var));

        // Normal case.
        $input = new InputStream(hex2bin('808001'));
        $this->assertTrue($input->readVarint32($var));
        $this->assertSame(16384, $var);
        $this->assertFalse($input->readVarint32($var));

        // Read two varint.
        $input = new InputStream(hex2bin('808001808002'));
        $this->assertTrue($input->readVarint32($var));
        $this->assertSame(16384, $var);
        $this->assertTrue($input->readVarint32($var));
        $this->assertSame(32768, $var);
        $this->assertFalse($input->readVarint32($var));

        // Read a 64-bit integer. High-order bits should be discarded.
        $input = new InputStream(hex2bin('808081808001'));
        $this->assertTrue($input->readVarint32($var));
        $this->assertSame(16384, $var);
        $this->assertFalse($input->readVarint32($var));
    }

    public function testReadTag()
    {
        $input = new InputStream(hex2bin('808001'));
        $tag = $input->readTag();
        $this->assertSame(16384, $tag);
        $tag = $input->readTag();
        $this->assertSame(0, $tag);
    }

    public function testPushPopLimit()
    {
        $input = new InputStream(hex2bin('808001'));
        $old_limit = $input->pushLimit(0);
        $tag = $input->readTag();
        $this->assertSame(0, $tag);
        $input->popLimit($old_limit);
        $tag = $input->readTag();
        $this->assertSame(16384, $tag);
    }

    public function testReadRaw()
    {
        $input = new InputStream(hex2bin('808001'));
        $buffer = null;

        $this->assertTrue($input->readRaw(3, $buffer));
        $this->assertSame(hex2bin('808001'), $buffer);

        $this->assertFalse($input->readRaw(1, $buffer));
    }

    public function testWriteVarint32()
    {
        $output = new OutputStream(3);
        $output->writeVarint32(16384);
        $this->assertSame(hex2bin('808001'), $output->getData());
    }

    public function testWriteVarint64()
    {
        $output = new OutputStream(10);
        $output->writeVarint64(-43);
        $this->assertSame(hex2bin('D5FFFFFFFFFFFFFFFF01'), $output->getData());
    }

    public function testWriteLittleEndian32()
    {
        $output = new OutputStream(4);
        $output->writeLittleEndian32(46);
        $this->assertSame(hex2bin('2E000000'), $output->getData());
    }

    public function testWriteLittleEndian64()
    {
        $output = new OutputStream(8);
        $output->writeLittleEndian64(47);
        $this->assertSame(hex2bin('2F00000000000000'), $output->getData());
    }

    public function testByteSize()
    {
        $m = new TestMessage();
        TestUtil::setTestMessage($m);
        $this->assertSame(447, $m->byteSize());
    }

    public function testPackedByteSize()
    {
        $m = new TestPackedMessage();
        TestUtil::setTestPackedMessage($m);
        $this->assertSame(156, $m->byteSize());
    }
}
