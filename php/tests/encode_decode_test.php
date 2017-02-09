<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\RepeatedField;
use Google\Protobuf\GPBType;
use Foo\TestEnum;
use Foo\TestMessage;
use Foo\TestMessage_Sub;
use Foo\TestPackedMessage;
use Foo\TestUnpackedMessage;

class EncodeDecodeTest extends TestBase
{

    public function testEncode()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);
        $this->setFields($from);
        $this->expectFields($from);

        $data = $from->encode();
        $this->assertSame(bin2hex(TestUtil::getGoldenTestMessage()),
                          bin2hex($data));
    }

    public function testDecode()
    {
        $to = new TestMessage();
        $to->decode(TestUtil::getGoldenTestMessage());
        $this->expectFields($to);
    }

    public function testEncodeDecode()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);
        $this->setFields($from);
        $this->expectFields($from);

        $data = $from->encode();

        $to = new TestMessage();
        $to->decode($data);
        $this->expectFields($to);
    }

    public function testEncodeDecodeEmpty()
    {
        $from = new TestMessage();
        $this->expectEmptyFields($from);

        $data = $from->encode();

        $to = new TestMessage();
        $to->decode($data);
        $this->expectEmptyFields($to);
    }

    public function testEncodeDecodeOneof()
    {
        $m = new TestMessage();

        $m->setOneofInt32(1);
        $data = $m->encode();
        $n = new TestMessage();
        $n->decode($data);
        $this->assertSame(1, $n->getOneofInt32());

        $m->setOneofFloat(2.0);
        $data = $m->encode();
        $n = new TestMessage();
        $n->decode($data);
        $this->assertSame(2.0, $n->getOneofFloat());

        $m->setOneofString('abc');
        $data = $m->encode();
        $n = new TestMessage();
        $n->decode($data);
        $this->assertSame('abc', $n->getOneofString());

        $sub_m = new TestMessage_Sub();
        $sub_m->setA(1);
        $m->setOneofMessage($sub_m);
        $data = $m->encode();
        $n = new TestMessage();
        $n->decode($data);
        $this->assertSame(1, $n->getOneofMessage()->getA());
    }

    public function testPackedEncode()
    {
        $from = new TestPackedMessage();
        TestUtil::setTestPackedMessage($from);
        $this->assertSame(TestUtil::getGoldenTestPackedMessage(),
                          $from->encode());
    }

    public function testPackedDecodePacked()
    {
        $to = new TestPackedMessage();
        $to->decode(TestUtil::getGoldenTestPackedMessage());
        TestUtil::assertTestPackedMessage($to);
    }

    public function testPackedDecodeUnpacked()
    {
        $to = new TestPackedMessage();
        $to->decode(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
    }

    public function testUnpackedEncode()
    {
        $from = new TestUnpackedMessage();
        TestUtil::setTestPackedMessage($from);
        $this->assertSame(TestUtil::getGoldenTestUnpackedMessage(),
                          $from->encode());
    }

    public function testUnpackedDecodePacked()
    {
        $to = new TestUnpackedMessage();
        $to->decode(TestUtil::getGoldenTestPackedMessage());
        TestUtil::assertTestPackedMessage($to);
    }

    public function testUnpackedDecodeUnpacked()
    {
        $to = new TestUnpackedMessage();
        $to->decode(TestUtil::getGoldenTestUnpackedMessage());
        TestUtil::assertTestPackedMessage($to);
    }

    public function testDecodeInt64()
    {
        // Read 64 testing
        $testVals = array(
            '10'                 => '100a',
            '100'                => '1064',
            '800'                => '10a006',
            '6400'               => '108032',
            '70400'              => '1080a604',
            '774400'             => '1080a22f',
            '9292800'            => '108098b704',
            '74342400'           => '1080c0b923',
            '743424000'          => '108080bfe202',
            '8177664000'         => '108080b5bb1e',
            '65421312000'        => '108080a8dbf301',
            '785055744000'       => '108080e0c7ec16',
            '9420668928000'      => '10808080dd969202',
            '103627358208000'    => '10808080fff9c717',
            '1139900940288000'   => '10808080f5bd978302',
            '13678811283456000'  => '10808080fce699a618',
            '109430490267648000' => '10808080e0b7ceb1c201',
            '984874412408832000' => '10808080e0f5c1bed50d',
        );

        $msg = new TestMessage();
        foreach ($testVals as $original => $encoded) {
            $msg->setOptionalInt64($original);
            $data = $msg->encode();
            $this->assertSame($encoded, bin2hex($data));
            $msg->setOptionalInt64(0);
            $msg->decode($data);
            $this->assertEquals($original, $msg->getOptionalInt64());
        }
    }

    public function testDecodeFieldNonExist()
    {
        $data = hex2bin('c80501');
        $m = new TestMessage();
        $m->decode($data);
    }

    # TODO(teboring): Add test back when php implementation is ready for json
    # encode/decode.
    # public function testJsonEncode()
    # {
    #     $from = new TestMessage();
    #     $this->setFields($from);
    #     $data = $from->jsonEncode();
    #     $to = new TestMessage();
    #     $to->jsonDecode($data);
    #     $this->expectFields($to);
    # }
}
