<?php

use Foo\TestEnum;
use Foo\TestMessage;
use Foo\TestMessage\Sub;
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

define('MIN_INT32', (int)-2147483648);
define('MIN_INT32_FLOAT', -2147483648.0);
define('MIN_INT32_STRING', '-2147483648');

define('MAX_UINT32', 4294967295);
define('MAX_UINT32_FLOAT', 4294967295.0);
define('MAX_UINT32_STRING', '4294967295');

define('MIN_UINT32', (int)-2147483648);
define('MIN_UINT32_FLOAT', -2147483648.0);
define('MIN_UINT32_STRING', '-2147483648');

define('MAX_INT64_STRING',  '9223372036854775807');
define('MIN_INT64_STRING',  '-9223372036854775808');
define('MAX_UINT64_STRING', '-9223372036854775808');

if (PHP_INT_SIZE === 8) {
    define('MAX_INT64',  (int)9223372036854775807);
    define('MIN_INT64',  (int)-9223372036854775808);
    define('MAX_UINT64', (int)-9223372036854775808);
} else {
    define('MAX_INT64', MAX_INT64_STRING);
    define('MIN_INT64', MIN_INT64_STRING);
    define('MAX_UINT64', MAX_UINT64_STRING);
}

class TestUtil
{

    public static function setTestMessage(TestMessage $m)
    {
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
        $m->setOptionalBytes('bbbb');
        $m->setOptionalEnum(TestEnum::ONE);
        $sub = new Sub();
        $m->setOptionalMessage($sub);
        $m->getOptionalMessage()->SetA(33);

        self::appendHelper($m, 'RepeatedInt32',    -42);
        self::appendHelper($m, 'RepeatedInt64',    -43);
        self::appendHelper($m, 'RepeatedUint32',    42);
        self::appendHelper($m, 'RepeatedUint64',    43);
        self::appendHelper($m, 'RepeatedSint32',   -44);
        self::appendHelper($m, 'RepeatedSint64',   -45);
        self::appendHelper($m, 'RepeatedFixed32',   46);
        self::appendHelper($m, 'RepeatedFixed64',   47);
        self::appendHelper($m, 'RepeatedSfixed32', -46);
        self::appendHelper($m, 'RepeatedSfixed64', -47);
        self::appendHelper($m, 'RepeatedFloat',    1.5);
        self::appendHelper($m, 'RepeatedDouble',   1.6);
        self::appendHelper($m, 'RepeatedBool',     true);
        self::appendHelper($m, 'RepeatedString',   'a');
        self::appendHelper($m, 'RepeatedBytes',    'bbbb');
        self::appendHelper($m, 'RepeatedEnum',     TestEnum::ZERO);
        self::appendHelper($m, 'RepeatedMessage',  new Sub());
        $m->getRepeatedMessage()[0]->setA(34);

        self::appendHelper($m, 'RepeatedInt32',    -52);
        self::appendHelper($m, 'RepeatedInt64',    -53);
        self::appendHelper($m, 'RepeatedUint32',    52);
        self::appendHelper($m, 'RepeatedUint64',    53);
        self::appendHelper($m, 'RepeatedSint32',   -54);
        self::appendHelper($m, 'RepeatedSint64',   -55);
        self::appendHelper($m, 'RepeatedFixed32',   56);
        self::appendHelper($m, 'RepeatedFixed64',   57);
        self::appendHelper($m, 'RepeatedSfixed32', -56);
        self::appendHelper($m, 'RepeatedSfixed64', -57);
        self::appendHelper($m, 'RepeatedFloat',    2.5);
        self::appendHelper($m, 'RepeatedDouble',   2.6);
        self::appendHelper($m, 'RepeatedBool',     false);
        self::appendHelper($m, 'RepeatedString',   'c');
        self::appendHelper($m, 'RepeatedBytes',    'dddd');
        self::appendHelper($m, 'RepeatedEnum',     TestEnum::ONE);
        self::appendHelper($m, 'RepeatedMessage',  new Sub());
        $m->getRepeatedMessage()[1]->SetA(35);

        self::kvUpdateHelper($m, 'MapInt32Int32', -62, -62);
        self::kvUpdateHelper($m, 'MapInt64Int64', -63, -63);
        self::kvUpdateHelper($m, 'MapUint32Uint32', 62, 62);
        self::kvUpdateHelper($m, 'MapUint64Uint64', 63, 63);
        self::kvUpdateHelper($m, 'MapSint32Sint32', -64, -64);
        self::kvUpdateHelper($m, 'MapSint64Sint64', -65, -65);
        self::kvUpdateHelper($m, 'MapFixed32Fixed32', 66, 66);
        self::kvUpdateHelper($m, 'MapFixed64Fixed64', 67, 67);
        self::kvUpdateHelper($m, 'MapSfixed32Sfixed32', -68, -68);
        self::kvUpdateHelper($m, 'MapSfixed64Sfixed64', -69, -69);
        self::kvUpdateHelper($m, 'MapInt32Float', 1, 3.5);
        self::kvUpdateHelper($m, 'MapInt32Double', 1, 3.6);
        self::kvUpdateHelper($m, 'MapBoolBool', true, true);
        self::kvUpdateHelper($m, 'MapStringString', 'e', 'e');
        self::kvUpdateHelper($m, 'MapInt32Bytes', 1, 'ffff');
        self::kvUpdateHelper($m, 'MapInt32Enum', 1, TestEnum::ONE);
        self::kvUpdateHelper($m, 'MapInt32Message', 1, new Sub());
        $m->getMapInt32Message()[1]->SetA(36);
    }

    public static function setTestMessage2(TestMessage $m)
    {
        $sub = new Sub();

        $m->setOptionalInt32(-142);
        $m->setOptionalInt64(-143);
        $m->setOptionalUint32(142);
        $m->setOptionalUint64(143);
        $m->setOptionalSint32(-144);
        $m->setOptionalSint64(-145);
        $m->setOptionalFixed32(146);
        $m->setOptionalFixed64(147);
        $m->setOptionalSfixed32(-146);
        $m->setOptionalSfixed64(-147);
        $m->setOptionalFloat(11.5);
        $m->setOptionalDouble(11.6);
        $m->setOptionalBool(true);
        $m->setOptionalString('aa');
        $m->setOptionalBytes('bb');
        $m->setOptionalEnum(TestEnum::TWO);
        $m->setOptionalMessage($sub);
        $m->getOptionalMessage()->SetA(133);

        self::appendHelper($m, 'RepeatedInt32',    -142);
        self::appendHelper($m, 'RepeatedInt64',    -143);
        self::appendHelper($m, 'RepeatedUint32',    142);
        self::appendHelper($m, 'RepeatedUint64',    143);
        self::appendHelper($m, 'RepeatedSint32',   -144);
        self::appendHelper($m, 'RepeatedSint64',   -145);
        self::appendHelper($m, 'RepeatedFixed32',   146);
        self::appendHelper($m, 'RepeatedFixed64',   147);
        self::appendHelper($m, 'RepeatedSfixed32', -146);
        self::appendHelper($m, 'RepeatedSfixed64', -147);
        self::appendHelper($m, 'RepeatedFloat',    11.5);
        self::appendHelper($m, 'RepeatedDouble',   11.6);
        self::appendHelper($m, 'RepeatedBool',     false);
        self::appendHelper($m, 'RepeatedString',   'aa');
        self::appendHelper($m, 'RepeatedBytes',    'bb');
        self::appendHelper($m, 'RepeatedEnum',     TestEnum::TWO);
        self::appendHelper($m, 'RepeatedMessage',  new Sub());
        $m->getRepeatedMessage()[0]->setA(134);

        self::kvUpdateHelper($m, 'MapInt32Int32', -62, -162);
        self::kvUpdateHelper($m, 'MapInt64Int64', -63, -163);
        self::kvUpdateHelper($m, 'MapUint32Uint32', 62, 162);
        self::kvUpdateHelper($m, 'MapUint64Uint64', 63, 163);
        self::kvUpdateHelper($m, 'MapSint32Sint32', -64, -164);
        self::kvUpdateHelper($m, 'MapSint64Sint64', -65, -165);
        self::kvUpdateHelper($m, 'MapFixed32Fixed32', 66, 166);
        self::kvUpdateHelper($m, 'MapFixed64Fixed64', 67, 167);
        self::kvUpdateHelper($m, 'MapSfixed32Sfixed32', -68, -168);
        self::kvUpdateHelper($m, 'MapSfixed64Sfixed64', -69, -169);
        self::kvUpdateHelper($m, 'MapInt32Float', 1, 13.5);
        self::kvUpdateHelper($m, 'MapInt32Double', 1, 13.6);
        self::kvUpdateHelper($m, 'MapBoolBool', true, false);
        self::kvUpdateHelper($m, 'MapStringString', 'e', 'ee');
        self::kvUpdateHelper($m, 'MapInt32Bytes', 1, 'ff');
        self::kvUpdateHelper($m, 'MapInt32Enum', 1, TestEnum::TWO);
        self::kvUpdateHelper($m, 'MapInt32Message', 1, new Sub());
        $m->getMapInt32Message()[1]->SetA(136);

        self::kvUpdateHelper($m, 'MapInt32Int32', -162, -162);
        self::kvUpdateHelper($m, 'MapInt64Int64', -163, -163);
        self::kvUpdateHelper($m, 'MapUint32Uint32', 162, 162);
        self::kvUpdateHelper($m, 'MapUint64Uint64', 163, 163);
        self::kvUpdateHelper($m, 'MapSint32Sint32', -164, -164);
        self::kvUpdateHelper($m, 'MapSint64Sint64', -165, -165);
        self::kvUpdateHelper($m, 'MapFixed32Fixed32', 166, 166);
        self::kvUpdateHelper($m, 'MapFixed64Fixed64', 167, 167);
        self::kvUpdateHelper($m, 'MapSfixed32Sfixed32', -168, -168);
        self::kvUpdateHelper($m, 'MapSfixed64Sfixed64', -169, -169);
        self::kvUpdateHelper($m, 'MapInt32Float', 2, 13.5);
        self::kvUpdateHelper($m, 'MapInt32Double', 2, 13.6);
        self::kvUpdateHelper($m, 'MapBoolBool', false, false);
        self::kvUpdateHelper($m, 'MapStringString', 'ee', 'ee');
        self::kvUpdateHelper($m, 'MapInt32Bytes', 2, 'ff');
        self::kvUpdateHelper($m, 'MapInt32Enum', 2, TestEnum::TWO);
        self::kvUpdateHelper($m, 'MapInt32Message', 2, new Sub());
        $m->getMapInt32Message()[2]->SetA(136);
    }

    public static function assertTestMessage(TestMessage $m)
    {
        if (PHP_INT_SIZE == 4) {
            assert('-43' === $m->getOptionalInt64());
            assert('43'  === $m->getOptionalUint64());
            assert('-45' === $m->getOptionalSint64());
            assert('47'  === $m->getOptionalFixed64());
            assert('-47' === $m->getOptionalSfixed64());
        } else {
            assert(-43 === $m->getOptionalInt64());
            assert(43  === $m->getOptionalUint64());
            assert(-45 === $m->getOptionalSint64());
            assert(47  === $m->getOptionalFixed64());
            assert(-47 === $m->getOptionalSfixed64());
        }
        assert(-42 === $m->getOptionalInt32());
        assert(42  === $m->getOptionalUint32());
        assert(-44 === $m->getOptionalSint32());
        assert(46  === $m->getOptionalFixed32());
        assert(-46 === $m->getOptionalSfixed32());
        assert(1.5 === $m->getOptionalFloat());
        assert(1.6 === $m->getOptionalDouble());
        assert(true=== $m->getOptionalBool());
        assert('a' === $m->getOptionalString());
        assert('bbbb' === $m->getOptionalBytes());
        assert(TestEnum::ONE === $m->getOptionalEnum());
        assert(33  === $m->getOptionalMessage()->getA());

        if (PHP_INT_SIZE == 4) {
            assert('-43' === $m->getRepeatedInt64()[0]);
            assert('43'  === $m->getRepeatedUint64()[0]);
            assert('-45' === $m->getRepeatedSint64()[0]);
            assert('47'  === $m->getRepeatedFixed64()[0]);
            assert('-47' === $m->getRepeatedSfixed64()[0]);
        } else {
            assert(-43 === $m->getRepeatedInt64()[0]);
            assert(43  === $m->getRepeatedUint64()[0]);
            assert(-45 === $m->getRepeatedSint64()[0]);
            assert(47  === $m->getRepeatedFixed64()[0]);
            assert(-47 === $m->getRepeatedSfixed64()[0]);
        }
        assert(-42 === $m->getRepeatedInt32()[0]);
        assert(42  === $m->getRepeatedUint32()[0]);
        assert(-44 === $m->getRepeatedSint32()[0]);
        assert(46  === $m->getRepeatedFixed32()[0]);
        assert(-46 === $m->getRepeatedSfixed32()[0]);
        assert(1.5 === $m->getRepeatedFloat()[0]);
        assert(1.6 === $m->getRepeatedDouble()[0]);
        assert(true=== $m->getRepeatedBool()[0]);
        assert('a' === $m->getRepeatedString()[0]);
        assert('bbbb' === $m->getRepeatedBytes()[0]);
        assert(TestEnum::ZERO === $m->getRepeatedEnum()[0]);
        assert(34  === $m->getRepeatedMessage()[0]->getA());

        if (PHP_INT_SIZE == 4) {
            assert('-53' === $m->getRepeatedInt64()[1]);
            assert('53'  === $m->getRepeatedUint64()[1]);
            assert('-55' === $m->getRepeatedSint64()[1]);
            assert('57'  === $m->getRepeatedFixed64()[1]);
            assert('-57' === $m->getRepeatedSfixed64()[1]);
        } else {
            assert(-53 === $m->getRepeatedInt64()[1]);
            assert(53  === $m->getRepeatedUint64()[1]);
            assert(-55 === $m->getRepeatedSint64()[1]);
            assert(57  === $m->getRepeatedFixed64()[1]);
            assert(-57 === $m->getRepeatedSfixed64()[1]);
        }
        assert(-52 === $m->getRepeatedInt32()[1]);
        assert(52  === $m->getRepeatedUint32()[1]);
        assert(-54 === $m->getRepeatedSint32()[1]);
        assert(56  === $m->getRepeatedFixed32()[1]);
        assert(-56 === $m->getRepeatedSfixed32()[1]);
        assert(2.5 === $m->getRepeatedFloat()[1]);
        assert(2.6 === $m->getRepeatedDouble()[1]);
        assert(false === $m->getRepeatedBool()[1]);
        assert('c' === $m->getRepeatedString()[1]);
        assert('dddd' === $m->getRepeatedBytes()[1]);
        assert(TestEnum::ONE === $m->getRepeatedEnum()[1]);
        assert(35  === $m->getRepeatedMessage()[1]->getA());

        if (PHP_INT_SIZE == 4) {
            assert('-63' === $m->getMapInt64Int64()['-63']);
            assert('63'  === $m->getMapUint64Uint64()['63']);
            assert('-65' === $m->getMapSint64Sint64()['-65']);
            assert('67'  === $m->getMapFixed64Fixed64()['67']);
            assert('-69'  === $m->getMapSfixed64Sfixed64()['-69']);
        } else {
            assert(-63 === $m->getMapInt64Int64()[-63]);
            assert(63  === $m->getMapUint64Uint64()[63]);
            assert(-65 === $m->getMapSint64Sint64()[-65]);
            assert(67  === $m->getMapFixed64Fixed64()[67]);
            assert(-69  === $m->getMapSfixed64Sfixed64()[-69]);
        }
        assert(-62 === $m->getMapInt32Int32()[-62]);
        assert(62  === $m->getMapUint32Uint32()[62]);
        assert(-64 === $m->getMapSint32Sint32()[-64]);
        assert(66  === $m->getMapFixed32Fixed32()[66]);
        assert(-68  === $m->getMapSfixed32Sfixed32()[-68]);
        assert(3.5 === $m->getMapInt32Float()[1]);
        assert(3.6 === $m->getMapInt32Double()[1]);
        assert(true === $m->getMapBoolBool()[true]);
        assert('e' === $m->getMapStringString()['e']);
        assert('ffff' === $m->getMapInt32Bytes()[1]);
        assert(TestEnum::ONE === $m->getMapInt32Enum()[1]);
        assert(36  === $m->getMapInt32Message()[1]->GetA());
    }

    public static function getGoldenTestMessage()
    {
        return hex2bin(
            "08D6FFFFFFFFFFFFFFFF01" .
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
            "7A0462626262" .
            "800101" .
            "8A01020821" .

            "FA0114D6FFFFFFFFFFFFFFFF01CCFFFFFFFFFFFFFFFF01" .
            "820214D5FFFFFFFFFFFFFFFF01CBFFFFFFFFFFFFFFFF01" .
            "8A02022A34" .
            "9202022B35" .
            "9A0202576B" .
            "A20202596D" .
            "AA02082E00000038000000" .
            "B202102F000000000000003900000000000000" .
            "BA0208D2FFFFFFC8FFFFFF" .
            "C20210D1FFFFFFFFFFFFFFC7FFFFFFFFFFFFFF" .
            "CA02080000C03F00002040" .
            "D202109A9999999999F93FCDCCCCCCCCCC0440" .
            "DA02020100" .
            "E2020161" .
            "E2020163" .
            "EA020462626262" .
            "EA020464646464" .
            "F202020001" .
            "FA02020822" .
            "FA02020823" .

            "BA041608C2FFFFFFFFFFFFFFFF0110C2FFFFFFFFFFFFFFFF01" .
            "C2041608C1FFFFFFFFFFFFFFFF0110C1FFFFFFFFFFFFFFFF01" .
            "CA0404083E103E" .
            "D20404083F103F" .
            "DA0404087f107F" .
            "E20406088101108101" .
            "EA040A0D420000001542000000" .
            "F20412094300000000000000114300000000000000" .
            "FA040A0DBCFFFFFF15BCFFFFFF" .
            "82051209BBFFFFFFFFFFFFFF11BBFFFFFFFFFFFFFF" .
            "8A050708011500006040" .
            "92050B080111CDCCCCCCCCCC0C40" .
            "9A050408011001" .
            "A205060a0165120165" .
            "AA05080801120466666666" .
            "B2050408011001" .
            "Ba0506080112020824"
        );
    }

    public static function setTestPackedMessage($m)
    {
        self::appendHelper($m, 'RepeatedInt32', -42);
        self::appendHelper($m, 'RepeatedInt32', -52);
        self::appendHelper($m, 'RepeatedInt64', -43);
        self::appendHelper($m, 'RepeatedInt64', -53);
        self::appendHelper($m, 'RepeatedUint32', 42);
        self::appendHelper($m, 'RepeatedUint32', 52);
        self::appendHelper($m, 'RepeatedUint64', 43);
        self::appendHelper($m, 'RepeatedUint64', 53);
        self::appendHelper($m, 'RepeatedSint32', -44);
        self::appendHelper($m, 'RepeatedSint32', -54);
        self::appendHelper($m, 'RepeatedSint64', -45);
        self::appendHelper($m, 'RepeatedSint64', -55);
        self::appendHelper($m, 'RepeatedFixed32', 46);
        self::appendHelper($m, 'RepeatedFixed32', 56);
        self::appendHelper($m, 'RepeatedFixed64', 47);
        self::appendHelper($m, 'RepeatedFixed64', 57);
        self::appendHelper($m, 'RepeatedSfixed32', -46);
        self::appendHelper($m, 'RepeatedSfixed32', -56);
        self::appendHelper($m, 'RepeatedSfixed64', -47);
        self::appendHelper($m, 'RepeatedSfixed64', -57);
        self::appendHelper($m, 'RepeatedFloat', 1.5);
        self::appendHelper($m, 'RepeatedFloat', 2.5);
        self::appendHelper($m, 'RepeatedDouble', 1.6);
        self::appendHelper($m, 'RepeatedDouble', 2.6);
        self::appendHelper($m, 'RepeatedBool', true);
        self::appendHelper($m, 'RepeatedBool', false);
        self::appendHelper($m, 'RepeatedEnum', TestEnum::ONE);
        self::appendHelper($m, 'RepeatedEnum', TestEnum::ZERO);
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
        assert(42  === $m->getRepeatedUint32()[0]);
        assert(52  === $m->getRepeatedUint32()[1]);
        assert(-44 === $m->getRepeatedSint32()[0]);
        assert(-54 === $m->getRepeatedSint32()[1]);
        assert(46  === $m->getRepeatedFixed32()[0]);
        assert(56  === $m->getRepeatedFixed32()[1]);
        assert(-46 === $m->getRepeatedSfixed32()[0]);
        assert(-56 === $m->getRepeatedSfixed32()[1]);
        assert(1.5 === $m->getRepeatedFloat()[0]);
        assert(2.5 === $m->getRepeatedFloat()[1]);
        assert(1.6 === $m->getRepeatedDouble()[0]);
        assert(2.6 === $m->getRepeatedDouble()[1]);
        assert(true  === $m->getRepeatedBool()[0]);
        assert(false === $m->getRepeatedBool()[1]);
        assert(TestEnum::ONE  === $m->getRepeatedEnum()[0]);
        assert(TestEnum::ZERO === $m->getRepeatedEnum()[1]);
        if (PHP_INT_SIZE == 4) {
            assert('-43' === $m->getRepeatedInt64()[0]);
            assert('-53' === $m->getRepeatedInt64()[1]);
            assert('43'  === $m->getRepeatedUint64()[0]);
            assert('53'  === $m->getRepeatedUint64()[1]);
            assert('-45' === $m->getRepeatedSint64()[0]);
            assert('-55' === $m->getRepeatedSint64()[1]);
            assert('47'  === $m->getRepeatedFixed64()[0]);
            assert('57'  === $m->getRepeatedFixed64()[1]);
            assert('-47' === $m->getRepeatedSfixed64()[0]);
            assert('-57' === $m->getRepeatedSfixed64()[1]);
        } else {
            assert(-43 === $m->getRepeatedInt64()[0]);
            assert(-53 === $m->getRepeatedInt64()[1]);
            assert(43  === $m->getRepeatedUint64()[0]);
            assert(53  === $m->getRepeatedUint64()[1]);
            assert(-45 === $m->getRepeatedSint64()[0]);
            assert(-55 === $m->getRepeatedSint64()[1]);
            assert(47  === $m->getRepeatedFixed64()[0]);
            assert(57  === $m->getRepeatedFixed64()[1]);
            assert(-47 === $m->getRepeatedSfixed64()[0]);
            assert(-57 === $m->getRepeatedSfixed64()[1]);
        }
    }

    public static function getGoldenTestPackedMessage()
    {
        return hex2bin(
            "D20514D6FFFFFFFFFFFFFFFF01CCFFFFFFFFFFFFFFFF01" .
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
            "D005D6FFFFFFFFFFFFFFFF01D005CCFFFFFFFFFFFFFFFF01" .
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

    private static function appendHelper($obj, $func_suffix, $value)
    {
        $getter_function = 'get'.$func_suffix;
        $setter_function = 'set'.$func_suffix;

        $arr = $obj->$getter_function();
        $arr[] = $value;
        $obj->$setter_function($arr);
    }

    private static function kvUpdateHelper($obj, $func_suffix, $key, $value)
    {
        $getter_function = 'get'.$func_suffix;
        $setter_function = 'set'.$func_suffix;

        $arr = $obj->$getter_function();
        $arr[$key] = $value;
        $obj->$setter_function($arr);
    }
}
