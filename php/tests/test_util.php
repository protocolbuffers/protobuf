<?php

use Foo\TestEnum;
use Foo\TestMessage;
use Foo\TestMessage_Sub;
use Foo\TestPackedMessage;
use Foo\TestUnpackedMessage;

define('MAX_FLOAT_DIFF', 0.000001);

if (PHP_INT_SIZE == 8) {
    define('MAX_INT_STRING', '9223372036854775807');
    define('MAX_INT_UPPER_STRING', '9223372036854775808');
} else {
    define('MAX_INT_STRING', '2147483647');
    define('MAX_INT_UPPER_STRING', '2147483648');
}

define('MAX_INT32', 2147483647);
define('MAX_INT32_FLOAT', 2147483647.0);
define('MAX_INT32_STRING', '2147483647');

define('MIN_INT32', -2147483648);
define('MIN_INT32_FLOAT', -2147483648.0);
define('MIN_INT32_STRING', '-2147483648');

define('MAX_UINT32', 4294967295);
define('MAX_UINT32_FLOAT', 4294967295.0);
define('MAX_UINT32_STRING', '4294967295');

define('MIN_UINT32', -2147483648);
define('MIN_UINT32_FLOAT', -2147483648.0);
define('MIN_UINT32_STRING', '-2147483648');

define('MAX_INT64', 9223372036854775807);
define('MAX_INT64_STRING', '9223372036854775807');

define('MIN_INT64_STRING', '-9223372036854775808');
if (PHP_INT_SIZE === 8) {
    define('MIN_INT64', -9223372036854775808);
} else {
    define('MIN_INT64', MIN_INT64_STRING);
}

define('MAX_UINT64_STRING', '-9223372036854775808');
define('MAX_UINT64', MAX_UINT64_STRING);

class TestUtil
{

    public static function setTestMessage(TestMessage $m)
    {
        $sub = new TestMessage_Sub();

        $m->setOptionalInt32(-42);
        $m->setOptionalInt64(-43);
        $m->setOptionalUint32(42);
        $m->setOptionalUint64(43);
        $m->setOptionalSint32(-44);
        $m->setOptionalSint64(-45);
        $m->setOptionalFixed32(46);
        $m->setOptionalFixed64(47);
        $m->setOptionalSfixed32(-46);
        $m->setOptionalSfixed64(-47);
        $m->setOptionalFloat(1.5);
        $m->setOptionalDouble(1.6);
        $m->setOptionalBool(true);
        $m->setOptionalString('a');
        $m->setOptionalBytes('b');
        $m->setOptionalEnum(TestEnum::ONE);
        $m->setOptionalMessage($sub);
        $m->getOptionalMessage()->SetA(33);

        $m->getRepeatedInt32()    []= -42;
        $m->getRepeatedInt64()    []= -43;
        $m->getRepeatedUint32()   []=  42;
        $m->getRepeatedUint64()   []=  43;
        $m->getRepeatedSint32()   []= -44;
        $m->getRepeatedSint64()   []= -45;
        $m->getRepeatedFixed32()  []=  46;
        $m->getRepeatedFixed64()  []=  47;
        $m->getRepeatedSfixed32() []= -46;
        $m->getRepeatedSfixed64() []= -47;
        $m->getRepeatedFloat()    []= 1.5;
        $m->getRepeatedDouble()   []= 1.6;
        $m->getRepeatedBool()     []= true;
        $m->getRepeatedString()   []= 'a';
        $m->getRepeatedBytes()    []= 'b';
        $m->getRepeatedEnum()     []= TestEnum::ZERO;
        $m->getRepeatedMessage()  []= new TestMessage_Sub();
        $m->getRepeatedMessage()[0]->setA(34);

        $m->getRepeatedInt32()    []= -52;
        $m->getRepeatedInt64()    []= -53;
        $m->getRepeatedUint32()   []=  52;
        $m->getRepeatedUint64()   []=  53;
        $m->getRepeatedSint32()   []= -54;
        $m->getRepeatedSint64()   []= -55;
        $m->getRepeatedFixed32()  []=  56;
        $m->getRepeatedFixed64()  []=  57;
        $m->getRepeatedSfixed32() []= -56;
        $m->getRepeatedSfixed64() []= -57;
        $m->getRepeatedFloat()    []= 2.5;
        $m->getRepeatedDouble()   []= 2.6;
        $m->getRepeatedBool()     []= false;
        $m->getRepeatedString()   []= 'c';
        $m->getRepeatedBytes()    []= 'd';
        $m->getRepeatedEnum()     []= TestEnum::ONE;
        $m->getRepeatedMessage()  []= new TestMessage_Sub();
        $m->getRepeatedMessage()[1]->SetA(35);

        $m->getMapInt32Int32()[-62] = -62;
        $m->getMapInt64Int64()[-63] = -63;
        $m->getMapUint32Uint32()[62] = 62;
        $m->getMapUint64Uint64()[63] = 63;
        $m->getMapSint32Sint32()[-64] = -64;
        $m->getMapSint64Sint64()[-65] = -65;
        $m->getMapFixed32Fixed32()[66] = 66;
        $m->getMapFixed64Fixed64()[67] = 67;
        $m->getMapInt32Float()[1] = 3.5;
        $m->getMapInt32Double()[1] = 3.6;
        $m->getMapBoolBool()[true] = true;
        $m->getMapStringString()['e'] = 'e';
        $m->getMapInt32Bytes()[1] = 'f';
        $m->getMapInt32Enum()[1] = TestEnum::ONE;
        $m->getMapInt32Message()[1] = new TestMessage_Sub();
        $m->getMapInt32Message()[1]->SetA(36);
    }

    public static function assertTestMessage(TestMessage $m)
    {
        assert(-42 === $m->getOptionalInt32());
        assert(42  === $m->getOptionalUint32());
        assert(-43 === $m->getOptionalInt64());
        assert(43  === $m->getOptionalUint64());
        assert(-44 === $m->getOptionalSint32());
        assert(-45 === $m->getOptionalSint64());
        assert(46  === $m->getOptionalFixed32());
        assert(47  === $m->getOptionalFixed64());
        assert(-46 === $m->getOptionalSfixed32());
        assert(-47 === $m->getOptionalSfixed64());
        assert(1.5 === $m->getOptionalFloat());
        assert(1.6 === $m->getOptionalDouble());
        assert(true=== $m->getOptionalBool());
        assert('a' === $m->getOptionalString());
        assert('b' === $m->getOptionalBytes());
        assert(TestEnum::ONE === $m->getOptionalEnum());
        assert(33  === $m->getOptionalMessage()->getA());

        assert(-42 === $m->getRepeatedInt32()[0]);
        assert(42  === $m->getRepeatedUint32()[0]);
        assert(-43 === $m->getRepeatedInt64()[0]);
        assert(43  === $m->getRepeatedUint64()[0]);
        assert(-44 === $m->getRepeatedSint32()[0]);
        assert(-45 === $m->getRepeatedSint64()[0]);
        assert(46  === $m->getRepeatedFixed32()[0]);
        assert(47  === $m->getRepeatedFixed64()[0]);
        assert(-46 === $m->getRepeatedSfixed32()[0]);
        assert(-47 === $m->getRepeatedSfixed64()[0]);
        assert(1.5 === $m->getRepeatedFloat()[0]);
        assert(1.6 === $m->getRepeatedDouble()[0]);
        assert(true=== $m->getRepeatedBool()[0]);
        assert('a' === $m->getRepeatedString()[0]);
        assert('b' === $m->getRepeatedBytes()[0]);
        assert(TestEnum::ZERO === $m->getRepeatedEnum()[0]);
        assert(34  === $m->getRepeatedMessage()[0]->getA());

        assert(-52 === $m->getRepeatedInt32()[1]);
        assert(52  === $m->getRepeatedUint32()[1]);
        assert(-53 === $m->getRepeatedInt64()[1]);
        assert(53  === $m->getRepeatedUint64()[1]);
        assert(-54 === $m->getRepeatedSint32()[1]);
        assert(-55 === $m->getRepeatedSint64()[1]);
        assert(56  === $m->getRepeatedFixed32()[1]);
        assert(57  === $m->getRepeatedFixed64()[1]);
        assert(-56 === $m->getRepeatedSfixed32()[1]);
        assert(-57 === $m->getRepeatedSfixed64()[1]);
        assert(2.5 === $m->getRepeatedFloat()[1]);
        assert(2.6 === $m->getRepeatedDouble()[1]);
        assert(false === $m->getRepeatedBool()[1]);
        assert('c' === $m->getRepeatedString()[1]);
        assert('d' === $m->getRepeatedBytes()[1]);
        assert(TestEnum::ONE === $m->getRepeatedEnum()[1]);
        assert(35  === $m->getRepeatedMessage()[1]->getA());

        assert(-62 === $m->getMapInt32Int32()[-62]);
        assert(-63 === $m->getMapInt64Int64()[-63]);
        assert(62  === $m->getMapUint32Uint32()[62]);
        assert(63  === $m->getMapUint64Uint64()[63]);
        assert(-64 === $m->getMapSint32Sint32()[-64]);
        assert(-65 === $m->getMapSint64Sint64()[-65]);
        assert(66  === $m->getMapFixed32Fixed32()[66]);
        assert(67  === $m->getMapFixed64Fixed64()[67]);
        assert(3.5 === $m->getMapInt32Float()[1]);
        assert(3.6 === $m->getMapInt32Double()[1]);
        assert(true === $m->getMapBoolBool()[true]);
        assert('e' === $m->getMapStringString()['e']);
        assert('f' === $m->getMapInt32Bytes()[1]);
        assert(TestEnum::ONE === $m->getMapInt32Enum()[1]);
        assert(36  === $m->getMapInt32Message()[1]->GetA());
    }

    public static function getGoldenTestMessage()
    {
        return hex2bin(
            "08D6FFFFFF0F" .
            "10D5FFFFFFFFFFFFFFFF01" .
            "182A" .
            "202B" .
            "2857" .
            "3059" .
            "3D2E000000" .
            "412F00000000000000" .
            "4DD2FFFFFF" .
            "51D1FFFFFFFFFFFFFF" .
            "5D0000C03F" .
            "619A9999999999F93F" .
            "6801" .
            "720161" .
            "7A0162" .
            "800101" .
            "8A01020821" .

            "F801D6FFFFFF0F" .
            "F801CCFFFFFF0F" .
            "8002D5FFFFFFFFFFFFFFFF01" .
            "8002CBFFFFFFFFFFFFFFFF01" .
            "88022A" .
            "880234" .
            "90022B" .
            "900235" .
            "980257" .
            "98026B" .
            "A00259" .
            "A0026D" .
            "AD022E000000" .
            "AD0238000000" .
            "B1022F00000000000000" .
            "B1023900000000000000" .
            "BD02D2FFFFFF" .
            "BD02C8FFFFFF" .
            "C102D1FFFFFFFFFFFFFF" .
            "C102C7FFFFFFFFFFFFFF" .
            "CD020000C03F" .
            "CD0200002040" .
            "D1029A9999999999F93F" .
            "D102CDCCCCCCCCCC0440" .
            "D80201" .
            "D80200" .
            "E2020161" .
            "E2020163" .
            "EA020162" .
            "EA020164" .
            "F00200" .
            "F00201" .
            "FA02020822" .
            "FA02020823" .

            "BA040C08C2FFFFFF0F10C2FFFFFF0F" .
            "C2041608C1FFFFFFFFFFFFFFFF0110C1FFFFFFFFFFFFFFFF01" .
            "CA0404083E103E" .
            "D20404083F103F" .
            "DA0404087f107F" .
            "E20406088101108101" .
            "EA040A0D420000001542000000" .
            "F20412094300000000000000114300000000000000" .
            "8A050708011500006040" .
            "92050B080111CDCCCCCCCCCC0C40" .
            "9A050408011001" .
            "A205060a0165120165" .
            "AA05050801120166" .
            "B2050408011001" .
            "Ba0506080112020824"
        );
    }

    public static function setTestPackedMessage($m)
    {
        $m->getRepeatedInt32()[] = -42;
        $m->getRepeatedInt32()[] = -52;
        $m->getRepeatedInt64()[] = -43;
        $m->getRepeatedInt64()[] = -53;
        $m->getRepeatedUint32()[] = 42;
        $m->getRepeatedUint32()[] = 52;
        $m->getRepeatedUint64()[] = 43;
        $m->getRepeatedUint64()[] = 53;
        $m->getRepeatedSint32()[] = -44;
        $m->getRepeatedSint32()[] = -54;
        $m->getRepeatedSint64()[] = -45;
        $m->getRepeatedSint64()[] = -55;
        $m->getRepeatedFixed32()[] = 46;
        $m->getRepeatedFixed32()[] = 56;
        $m->getRepeatedFixed64()[] = 47;
        $m->getRepeatedFixed64()[] = 57;
        $m->getRepeatedSfixed32()[] = -46;
        $m->getRepeatedSfixed32()[] = -56;
        $m->getRepeatedSfixed64()[] = -47;
        $m->getRepeatedSfixed64()[] = -57;
        $m->getRepeatedFloat()[] = 1.5;
        $m->getRepeatedFloat()[] = 2.5;
        $m->getRepeatedDouble()[] = 1.6;
        $m->getRepeatedDouble()[] = 2.6;
        $m->getRepeatedBool()[] = true;
        $m->getRepeatedBool()[] = false;
        $m->getRepeatedEnum()[] = TestEnum::ONE;
        $m->getRepeatedEnum()[] = TestEnum::ZERO;
    }

    public static function assertTestPackedMessage($m)
    {
        assert(2 === count($m->getRepeatedInt32()));
        assert(2 === count($m->getRepeatedInt64()));
        assert(2 === count($m->getRepeatedUint32()));
        assert(2 === count($m->getRepeatedUint64()));
        assert(2 === count($m->getRepeatedSint32()));
        assert(2 === count($m->getRepeatedSint64()));
        assert(2 === count($m->getRepeatedFixed32()));
        assert(2 === count($m->getRepeatedFixed64()));
        assert(2 === count($m->getRepeatedSfixed32()));
        assert(2 === count($m->getRepeatedSfixed64()));
        assert(2 === count($m->getRepeatedFloat()));
        assert(2 === count($m->getRepeatedDouble()));
        assert(2 === count($m->getRepeatedBool()));
        assert(2 === count($m->getRepeatedEnum()));

        assert(-42 === $m->getRepeatedInt32()[0]);
        assert(-52 === $m->getRepeatedInt32()[1]);
        assert(-43 === $m->getRepeatedInt64()[0]);
        assert(-53 === $m->getRepeatedInt64()[1]);
        assert(42  === $m->getRepeatedUint32()[0]);
        assert(52  === $m->getRepeatedUint32()[1]);
        assert(43  === $m->getRepeatedUint64()[0]);
        assert(53  === $m->getRepeatedUint64()[1]);
        assert(-44 === $m->getRepeatedSint32()[0]);
        assert(-54 === $m->getRepeatedSint32()[1]);
        assert(-45 === $m->getRepeatedSint64()[0]);
        assert(-55 === $m->getRepeatedSint64()[1]);
        assert(46  === $m->getRepeatedFixed32()[0]);
        assert(56  === $m->getRepeatedFixed32()[1]);
        assert(47  === $m->getRepeatedFixed64()[0]);
        assert(57  === $m->getRepeatedFixed64()[1]);
        assert(-46 === $m->getRepeatedSfixed32()[0]);
        assert(-56 === $m->getRepeatedSfixed32()[1]);
        assert(-47 === $m->getRepeatedSfixed64()[0]);
        assert(-57 === $m->getRepeatedSfixed64()[1]);
        assert(1.5 === $m->getRepeatedFloat()[0]);
        assert(2.5 === $m->getRepeatedFloat()[1]);
        assert(1.6 === $m->getRepeatedDouble()[0]);
        assert(2.6 === $m->getRepeatedDouble()[1]);
        assert(true  === $m->getRepeatedBool()[0]);
        assert(false === $m->getRepeatedBool()[1]);
        assert(TestEnum::ONE  === $m->getRepeatedEnum()[0]);
        assert(TestEnum::ZERO === $m->getRepeatedEnum()[1]);
    }

    public static function getGoldenTestPackedMessage()
    {
        return hex2bin(
            "D2050AD6FFFFFF0FCCFFFFFF0F" .
            "DA0514D5FFFFFFFFFFFFFFFF01CBFFFFFFFFFFFFFFFF01" .
            "E205022A34" .
            "EA05022B35" .
            "F20502576B" .
            "FA0502596D" .
            "8206082E00000038000000" .
            "8A06102F000000000000003900000000000000" .
            "920608D2FFFFFFC8FFFFFF" .
            "9A0610D1FFFFFFFFFFFFFFC7FFFFFFFFFFFFFF" .
            "A206080000C03F00002040" .
            "AA06109A9999999999F93FCDCCCCCCCCCC0440" .
            "B206020100" .
            "BA06020100"
        );
    }

    public static function getGoldenTestUnpackedMessage()
    {
        return hex2bin(
            "D005D6FFFFFF0FD005CCFFFFFF0F" .
            "D805D5FFFFFFFFFFFFFFFF01D805CBFFFFFFFFFFFFFFFF01" .
            "E0052AE00534" .
            "E8052BE80535" .
            "F00557F0056B" .
            "F80559F8056D" .
            "85062E000000850638000000" .
            "89062F0000000000000089063900000000000000" .
            "9506D2FFFFFF9506C8FFFFFF" .
            "9906D1FFFFFFFFFFFFFF9906C7FFFFFFFFFFFFFF" .
            "A5060000C03FA50600002040" .
            "A9069A9999999999F93FA906CDCCCCCCCCCC0440" .
            "B00601B00600" .
            "B80601B80600"
        );
    }
}
