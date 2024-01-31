<?php

require_once('test_base.php');
require_once('test_util.php');

use Foo\TestMessage;

class GeneratedPhpdocTest extends TestBase
{
    public function testPhpDocForClass()
    {
        $class = new ReflectionClass('Foo\TestMessage');
        $doc = $class->getDocComment();
        $this->assertStringContains('foo.TestMessage', $doc);
    }

    public function testPhpDocForConstructor()
    {
        $class = new ReflectionClass('Foo\TestMessage');
        $doc = $class->getMethod('__construct')->getDocComment();
        $this->assertStringContains('@param array $data', $doc);
        $this->assertStringContains('@type int $optional_int32', $doc);
    }

    /**
     * @dataProvider providePhpDocForGettersAndSetters
     */
    public function testPhpDocForIntGetters($methods, $expectedDoc)
    {
        $class = new ReflectionClass('Foo\TestMessage');
        foreach ($methods as $method) {
            $doc = $class->getMethod($method)->getDocComment();
            $this->assertStringContains($expectedDoc, $doc);
        }
    }

    public function providePhpDocForGettersAndSetters()
    {
        return [
            [
                [
                    'setOptionalInt32',
                    'setOptionalUint32',
                    'setOptionalSint32',
                    'setOptionalFixed32',
                    'setOptionalSfixed32',
                    'setOneofInt32',
                    'setOneofUint32',
                    'setOneofSint32',
                    'setOneofFixed32',
                    'setOneofSfixed32',
                    'setOptionalEnum',
                    'setOptionalNoNamespaceEnum',
                    'setOptionalNestedEnum',
                    'setOneofEnum'
                ],
                '@param int $var'
            ],
            [
                [
                    'setOptionalInt64',
                    'setOptionalUint64',
                    'setOptionalSint64',
                    'setOptionalFixed64',
                    'setOptionalSfixed64',
                    'setOneofInt64',
                    'setOneofUint64',
                    'setOneofSint64',
                    'setOneofFixed64',
                    'setOneofSfixed64',
                ],
                '@param int|string $var'
            ],
            [
                [
                    'getOptionalInt32',
                    'getOptionalUint32',
                    'getOptionalSint32',
                    'getOptionalFixed32',
                    'getOptionalSfixed32',
                    'getOneofInt32',
                    'getOneofUint32',
                    'getOneofSint32',
                    'getOneofFixed32',
                    'getOneofSfixed32',
                    'getOptionalEnum',
                    'getOptionalNoNamespaceEnum',
                    'getOptionalNestedEnum',
                    'getOneofEnum',
                ],
                '@return int'
            ],
            [
                [
                    'setOptionalInt64',
                    'setOptionalUint64',
                    'setOptionalSint64',
                    'setOptionalFixed64',
                    'setOptionalSfixed64',
                    'setOneofInt64',
                    'setOneofUint64',
                    'setOneofSint64',
                    'setOneofFixed64',
                    'setOneofSfixed64',
                ],
                '@param int|string $var'
            ],
            [
                [
                    'getRepeatedInt32',
                    'getRepeatedUint32',
                    'getRepeatedSint32',
                    'getRepeatedFixed32',
                    'getRepeatedSfixed32',
                    'getRepeatedEnum',
                ],
                '@return array<int>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedInt64',
                    'getRepeatedUint64',
                    'getRepeatedSint64',
                    'getRepeatedSfixed64',
                    'getRepeatedFixed64',
                ],
                '@return array<int>|array<string>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedString',
                    'getRepeatedBytes',
                ],
                '@return array<string>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedFloat',
                    'getRepeatedDouble',
                ],
                '@return array<float>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedBool',
                ],
                '@return array<bool>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedMessage',
                ],
                '@return array<\Foo\TestMessage\Sub>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedRecursive',
                ],
                '@return array<\Foo\TestMessage>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedNoNamespaceMessage',
                ],
                '@return array<\NoNamespaceMessage>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getRepeatedNoNamespaceEnum',
                ],
                '@return array<int>|\Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getMapInt32Int32',
                    'getMapUint32Uint32',
                    'getMapSint32Sint32',
                    'getMapFixed32Fixed32',
                    'getMapSfixed32Sfixed32',
                    'getMapInt32Enum',
                ],
                '@return array<int,int>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapInt64Int64',
                    'getMapUint64Uint64',
                    'getMapSint64Sint64',
                    'getMapFixed64Fixed64',
                    'getMapSfixed64Sfixed64',
                ],
                '@return array<int|string,int|string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapInt32Float',
                    'getMapInt32Double',
                ],
                '@return array<int,float>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapBoolBool',
                ],
                '@return array<bool,bool>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapStringString',
                ],
                '@return array<string,string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapInt32Bytes',
                ],
                '@return array<int,string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapInt32Message',
                ],
                '@return array<int,\Foo\TestMessage\Sub>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getMapRecursive',
                ],
                '@return array<int,\Foo\TestMessage>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setRepeatedInt32',
                    'setRepeatedUint32',
                    'setRepeatedSint32',
                    'setRepeatedFixed32',
                    'setRepeatedSfixed32',
                    'setRepeatedEnum',
                    'setRepeatedNoNamespaceEnum',
                ],
                '@param array<int>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedInt64',
                    'setRepeatedUint64',
                    'setRepeatedSint64',
                    'setRepeatedFixed64',
                    'setRepeatedSfixed64',
                ],
                '@param array<int>|array<string>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedFloat',
                    'setRepeatedDouble',
                ],
                '@param array<float>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedBool',
                ],
                '@param array<bool>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedString',
                    'setRepeatedBytes',
                ],
                '@param array<string>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedMessage',
                ],
                '@param array<\Foo\TestMessage\Sub>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedRecursive',
                ],
                '@param array<\Foo\TestMessage>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedNoNamespaceMessage',
                ],
                '@param array<\NoNamespaceMessage>|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setMapInt32Int32',
                    'setMapUint32Uint32',
                    'setMapSint32Sint32',
                    'setMapFixed32Fixed32',
                    'setMapSfixed32Sfixed32',
                    'setMapInt32Enum',
                ],
                '@param array<int,int>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapInt64Int64',
                    'setMapUint64Uint64',
                    'setMapSint64Sint64',
                    'setMapFixed64Fixed64',
                    'setMapSfixed64Sfixed64',
                ],
                '@param array<int|string,int|string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapInt32Float',
                    'setMapInt32Double',
                ],
                '@param array<int,float>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapBoolBool',
                ],
                '@param array<bool,bool>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapStringString',
                ],
                '@param array<string,string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapInt32Bytes',
                ],
                '@param array<int,string>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapInt32Message',
                ],
                '@param array<int,\Foo\TestMessage\Sub>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'setMapRecursive',
                ],
                '@param array<int,\Foo\TestMessage>|\Google\Protobuf\Internal\MapField'
            ],
            [
                [
                    'getOptionalFloat',
                    'getOptionalDouble',
                    'getOneofDouble',
                    'getOneofFloat',
                ],
                '@return float'
            ],
            [
                [
                    'setOptionalFloat',
                    'setOptionalDouble',
                    'setOneofDouble',
                    'setOneofFloat',
                ],
                '@param float $var'
            ],
            [
                [
                    'getOptionalBool',
                    'getOneofBool',
                ],
                '@return bool'],
            [
                [
                    'setOptionalBool',
                    'setOneofBool',
                ],
                '@param bool $var'
            ],
            [
                [
                    'getOptionalString',
                    'getOptionalBytes',
                    'getOneofString',
                    'getOneofBytes',
                    'getMyOneof',
                ],
                '@return string'
            ],
            [
                [
                    'setOptionalString',
                    'setOptionalBytes',
                    'setOneofString',
                    'setOneofBytes',
                ],
                '@param string $var'
            ],

            [
                [
                    'getOptionalMessage',
                    'getOneofMessage'
                ],
                '@return \Foo\TestMessage\Sub'
            ],
            [
                [
                    'setOptionalMessage',
                    'setOneofMessage'
                ],
                '@param \Foo\TestMessage\Sub $var'
            ],
            [
                [
                    'getOptionalIncludedMessage'
                ],
                '@return \Bar\TestInclude'
            ],
            [
                [
                    'setOptionalIncludedMessage'
                ],
                '@param \Bar\TestInclude $var'
            ],
            [
                [
                    'getRecursive'
                ],
                '@return \Foo\TestMessage'
            ],
            [
                [
                    'setRecursive'
                ],
                '@param \Foo\TestMessage $var'
            ],

            [
                [
                    'getOptionalNoNamespaceMessage'
                ],
                '@return \NoNamespaceMessage'
            ],
            [
                [
                    'setOptionalNoNamespaceMessage'
                ],
                '@param \NoNamespaceMessage $var'
            ],
            [
                [
                    'setDeprecatedOptionalInt32',
                    'getDeprecatedOptionalInt32',
                ],
                '@deprecated'
            ],
        ];
    }
}
