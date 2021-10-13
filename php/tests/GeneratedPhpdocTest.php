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
                    'getRepeatedInt64',
                    'getRepeatedUint32',
                    'getRepeatedUint64',
                    'getRepeatedSint32',
                    'getRepeatedSint64',
                    'getRepeatedFixed32',
                    'getRepeatedFixed64',
                    'getRepeatedSfixed32',
                    'getRepeatedSfixed64',
                    'getRepeatedFloat',
                    'getRepeatedDouble',
                    'getRepeatedBool',
                    'getRepeatedString',
                    'getRepeatedBytes',
                    'getRepeatedEnum',
                    'getRepeatedMessage',
                    'getRepeatedRecursive',
                    'getRepeatedNoNamespaceMessage',
                    'getRepeatedNoNamespaceEnum',
                ],
                '@return \Google\Protobuf\Internal\RepeatedField'
            ],
            [
                [
                    'getMapInt32Int32',
                    'getMapInt64Int64',
                    'getMapUint32Uint32',
                    'getMapUint64Uint64',
                    'getMapSint32Sint32',
                    'getMapSint64Sint64',
                    'getMapFixed32Fixed32',
                    'getMapFixed64Fixed64',
                    'getMapSfixed32Sfixed32',
                    'getMapSfixed64Sfixed64',
                    'getMapInt32Float',
                    'getMapInt32Double',
                    'getMapBoolBool',
                    'getMapStringString',
                    'getMapInt32Bytes',
                    'getMapInt32Enum',
                    'getMapInt32Message',
                    'getMapRecursive',
                ],
                '@return \Google\Protobuf\Internal\MapField'
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
                '@param int[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedInt64',
                    'setRepeatedUint64',
                    'setRepeatedSint64',
                    'setRepeatedFixed64',
                    'setRepeatedSfixed64',
                ],
                '@param int[]|string[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedFloat',
                    'setRepeatedDouble',
                ],
                '@param float[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedBool',
                ],
                '@param bool[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedString',
                    'setRepeatedBytes',
                ],
                '@param string[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedMessage',
                ],
                '@param \Foo\TestMessage\Sub[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedRecursive',
                ],
                '@param \Foo\TestMessage[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setRepeatedNoNamespaceMessage',
                ],
                '@param \NoNamespaceMessage[]|\Google\Protobuf\Internal\RepeatedField $var'
            ],
            [
                [
                    'setMapInt32Int32',
                    'setMapInt64Int64',
                    'setMapUint32Uint32',
                    'setMapUint64Uint64',
                    'setMapSint32Sint32',
                    'setMapSint64Sint64',
                    'setMapFixed32Fixed32',
                    'setMapFixed64Fixed64',
                    'setMapSfixed32Sfixed32',
                    'setMapSfixed64Sfixed64',
                    'setMapInt32Float',
                    'setMapInt32Double',
                    'setMapBoolBool',
                    'setMapStringString',
                    'setMapInt32Bytes',
                    'setMapInt32Enum',
                    'setMapInt32Message',
                    'setMapRecursive',
                ],
                '@param array|\Google\Protobuf\Internal\MapField $var'
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
