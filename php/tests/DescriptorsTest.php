<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\DescriptorPool;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;
use Descriptors\TestDescriptorsEnum;
use Descriptors\TestDescriptorsMessage;
use Descriptors\TestDescriptorsMessage\Sub;
use Foo\TestMessage;
use Bar\TestInclude;

class DescriptorsTest extends TestBase
{

    // Redefine these here for compatibility with c extension
    const GPBLABEL_OPTIONAL = 1;
    const GPBLABEL_REQUIRED = 2;
    const GPBLABEL_REPEATED = 3;

    const GPBTYPE_DOUBLE   =  1;
    const GPBTYPE_FLOAT    =  2;
    const GPBTYPE_INT64    =  3;
    const GPBTYPE_UINT64   =  4;
    const GPBTYPE_INT32    =  5;
    const GPBTYPE_FIXED64  =  6;
    const GPBTYPE_FIXED32  =  7;
    const GPBTYPE_BOOL     =  8;
    const GPBTYPE_STRING   =  9;
    const GPBTYPE_GROUP    = 10;
    const GPBTYPE_MESSAGE  = 11;
    const GPBTYPE_BYTES    = 12;
    const GPBTYPE_UINT32   = 13;
    const GPBTYPE_ENUM     = 14;
    const GPBTYPE_SFIXED32 = 15;
    const GPBTYPE_SFIXED64 = 16;
    const GPBTYPE_SINT32   = 17;
    const GPBTYPE_SINT64   = 18;

    #########################################################
    # Test descriptor pool.
    #########################################################

    public function testDescriptorPool()
    {
        $pool = DescriptorPool::getGeneratedPool();

        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $this->assertInstanceOf('\Google\Protobuf\Descriptor', $desc);

        $enumDesc = $pool->getEnumDescriptorByClassName(get_class(new TestDescriptorsEnum()));
        $this->assertInstanceOf('\Google\Protobuf\EnumDescriptor', $enumDesc);
    }

    public function testDescriptorPoolIncorrectArgs()
    {
        $pool = DescriptorPool::getGeneratedPool();

        $desc = $pool->getDescriptorByClassName('NotAClass');
        $this->assertNull($desc);

        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsEnum()));
        $this->assertNull($desc);

        $enumDesc = $pool->getEnumDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $this->assertNull($enumDesc);
    }

    #########################################################
    # Test descriptor.
    #########################################################

    public function testDescriptor()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $class = get_class(new TestDescriptorsMessage());
        $this->assertSame('Descriptors\TestDescriptorsMessage', $class);
        $desc = $pool->getDescriptorByClassName($class);

        $this->assertSame('descriptors.TestDescriptorsMessage', $desc->getFullName());
        $this->assertSame($class, $desc->getClass());

        $this->assertInstanceOf('\Google\Protobuf\FieldDescriptor', $desc->getField(0));
        $this->assertSame(7, $desc->getFieldCount());

        $this->assertInstanceOf('\Google\Protobuf\OneofDescriptor', $desc->getOneofDecl(0));
        $this->assertSame(1, $desc->getOneofDeclCount());
    }

    public function testDescriptorForIncludedMessage()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $class = get_class(new TestMessage());
        $this->assertSame('Foo\TestMessage', $class);
        $desc = $pool->getDescriptorByClassName($class);
        $fielddesc = $desc->getField(17);
        $subdesc = $fielddesc->getMessageType();
        $this->assertSame('Bar\TestInclude', $subdesc->getClass());
    }

    #########################################################
    # Test enum descriptor.
    #########################################################

    public function testEnumDescriptor()
    {
        // WARNING - we need to do this so that TestDescriptorsEnum is registered!!?
        new TestDescriptorsMessage();

        $pool = DescriptorPool::getGeneratedPool();

        $enumDesc = $pool->getEnumDescriptorByClassName(get_class(new TestDescriptorsEnum()));

        // Build map of enum values
        $enumDescMap = [];
        for ($i = 0; $i < $enumDesc->getValueCount(); $i++) {
            $enumValueDesc = $enumDesc->getValue($i);
            $this->assertInstanceOf('\Google\Protobuf\EnumValueDescriptor', $enumValueDesc);
            $enumDescMap[$enumValueDesc->getNumber()] = $enumValueDesc->getName();
        }

        $this->assertSame('ZERO', $enumDescMap[0]);
        $this->assertSame('ONE', $enumDescMap[1]);

        $this->assertSame(2, $enumDesc->getValueCount());
    }

    #########################################################
    # Test field descriptor.
    #########################################################

    public function testFieldDescriptor()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));

        $fieldDescMap = $this->buildFieldMap($desc);

        // Optional int field
        $fieldDesc = $fieldDescMap[1];
        $this->assertSame('optional_int32', $fieldDesc->getName());
        $this->assertSame(1, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());

        // Optional enum field
        $fieldDesc = $fieldDescMap[16];
        $this->assertSame('optional_enum', $fieldDesc->getName());
        $this->assertSame(16, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_ENUM, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\EnumDescriptor', $fieldDesc->getEnumType());
        $this->assertFalse($fieldDesc->isMap());

        // Optional message field
        $fieldDesc = $fieldDescMap[17];
        $this->assertSame('optional_message', $fieldDesc->getName());
        $this->assertSame(17, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_MESSAGE, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\Descriptor', $fieldDesc->getMessageType());
        $this->assertFalse($fieldDesc->isMap());

        // Repeated int field
        $fieldDesc = $fieldDescMap[31];
        $this->assertSame('repeated_int32', $fieldDesc->getName());
        $this->assertSame(31, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_REPEATED, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());

        // Repeated message field
        $fieldDesc = $fieldDescMap[47];
        $this->assertSame('repeated_message', $fieldDesc->getName());
        $this->assertSame(47, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_REPEATED, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_MESSAGE, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\Descriptor', $fieldDesc->getMessageType());
        $this->assertFalse($fieldDesc->isMap());

        // Oneof int field
        // Tested further in testOneofDescriptor()
        $fieldDesc = $fieldDescMap[51];
        $this->assertSame('oneof_int32', $fieldDesc->getName());
        $this->assertSame(51, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());

        // Map int-enum field
        $fieldDesc = $fieldDescMap[71];
        $this->assertSame('map_int32_enum', $fieldDesc->getName());
        $this->assertSame(71, $fieldDesc->getNumber());
        $this->assertSame(self::GPBLABEL_REPEATED, $fieldDesc->getLabel());
        $this->assertSame(self::GPBTYPE_MESSAGE, $fieldDesc->getType());
        $this->assertTrue($fieldDesc->isMap());
        $mapDesc = $fieldDesc->getMessageType();
        $this->assertSame('descriptors.TestDescriptorsMessage.MapInt32EnumEntry', $mapDesc->getFullName());
        $this->assertSame(self::GPBTYPE_INT32, $mapDesc->getField(0)->getType());
        $this->assertSame(self::GPBTYPE_ENUM, $mapDesc->getField(1)->getType());
    }

    public function testFieldDescriptorEnumException()
    {
        $this->expectException(Exception::class);

        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $fieldDesc = $desc->getField(0);
        $fieldDesc->getEnumType();
    }

    public function testFieldDescriptorMessageException()
    {
        $this->expectException(Exception::class);

        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $fieldDesc = $desc->getField(0);
        $fieldDesc->getMessageType();
    }

    #########################################################
    # Test oneof descriptor.
    #########################################################

    public function testOneofDescriptor()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));

        $fieldDescMap = $this->buildFieldMap($desc);
        $fieldDesc = $fieldDescMap[51];

        $oneofDesc = $desc->getOneofDecl(0);

        $this->assertSame('my_oneof', $oneofDesc->getName());
        $fieldDescFromOneof = $oneofDesc->getField(0);
        $this->assertSame($fieldDesc, $fieldDescFromOneof);
        $this->assertSame(1, $oneofDesc->getFieldCount());
    }

    private function buildFieldMap($desc)
    {
        $fieldDescMap = [];
        for ($i = 0; $i < $desc->getFieldCount(); $i++) {
            $fieldDesc = $desc->getField($i);
            $fieldDescMap[$fieldDesc->getNumber()] = $fieldDesc;
        }
        return $fieldDescMap;
    }
}
