<?php

require_once('generated/Descriptors/TestDescriptorsEnum.php');
require_once('generated/Descriptors/TestDescriptorsMessage.php');
require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\DescriptorPool;
use Google\Protobuf\Internal\GPBLabel;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;
use Google\Protobuf\Internal\GPBType;
use Descriptors\TestDescriptorsEnum;
use Descriptors\TestDescriptorsMessage;
use Descriptors\TestDescriptorsMessage_Sub;

class DescriptorsTest extends TestBase
{

    #########################################################
    # Test descriptor pool.
    #########################################################

    public function testDescriptorPool()
    {
        $pool = DescriptorPool::getGeneratedPool();

        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $this->assertInstanceOf('\Google\Protobuf\Internal\Descriptor', $desc);

        $enumDesc = $pool->getEnumDescriptorByClassName(get_class(new TestDescriptorsEnum()));
        $this->assertInstanceOf('\Google\Protobuf\Internal\EnumDescriptor', $enumDesc);
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
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));

        $this->assertSame('descriptors.TestDescriptorsMessage', $desc->getFullName());

        $this->assertInstanceOf('\Google\Protobuf\Internal\FieldDescriptor', $desc->getField(0));
        $this->assertSame(12, $desc->getFieldCount());

        $this->assertInstanceOf('\Google\Protobuf\Internal\Descriptor', $desc->getNestedType(0));
        $this->assertSame(1, $this->getNestedTypeCount());

        $this->assertInstanceOf('\Google\Protobuf\Internal\EnumDescriptor', $desc->getEnumType(0));
        $this->assertSame(1, $this->getEnumTypeCount());

        $this->assertInstanceOf('\Google\Protobuf\Internal\OneofDescriptor', $desc->getOneofDecl(0));
        $this->assertSame(1, $this->getOneofDeclCount());
    }

    #########################################################
    # Test enum descriptor.
    #########################################################

    public function testEnumDescriptor()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $enumDesc = $pool->getEnumDescriptorByClassName(get_class(new TestDescriptorsEnum()));

        $enumValueDesc = $enumDesc->getValue(0);
        $this->assertInstanceOf('\Google\Protobuf\Internal\EnumValueDescriptor', $enumValueDesc);
        $this->assertSame(0, $enumValueDesc->getValue());
        $this->assertSame('ZERO', $enumValueDesc->getName());

        $enumValueDesc = $enumDesc->getValue(1);
        $this->assertInstanceOf('\Google\Protobuf\Internal\EnumValueDescriptor', $enumValueDesc);
        $this->assertSame(1, $enumValueDesc->getValue());
        $this->assertSame('ONE', $enumValueDesc->getName());

        $this->assertSame(2, $enumDesc->getValueCount());
    }

    #########################################################
    # Test field descriptor.
    #########################################################

    public function testFieldDescriptor()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));

        // Optional int field
        $fieldDesc = $desc->getField(0);
        $this->assertSame('optional_int32', $fieldDesc->getName());
        $this->assertSame(1, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(GPBType::INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());

        // Optional enum field
        $fieldDesc = $desc->getField(1);
        $this->assertSame('optional_enum', $fieldDesc->getName());
        $this->assertSame(16, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(GPBType::ENUM, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\Internal\EnumDescriptor', $fieldDesc->getEnumType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());

        // Optional message field
        $fieldDesc = $desc->getField(2);
        $this->assertSame('optional_message', $fieldDesc->getName());
        $this->assertSame(17, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(GPBType::MESSAGE, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\Internal\Descriptor', $fieldDesc->getMessageType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());

        // Repeated int field
        $fieldDesc = $desc->getField(3);
        $this->assertSame('repeated_int32', $fieldDesc->getName());
        $this->assertSame(31, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::REPEATED, $fieldDesc->getLabel());
        $this->assertSame(GPBType::INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());

        // Repeated message field
        $fieldDesc = $desc->getField(4);
        $this->assertSame('repeated_message', $fieldDesc->getName());
        $this->assertSame(47, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::REPEATED, $fieldDesc->getLabel());
        $this->assertSame(GPBType::MESSAGE, $fieldDesc->getType());
        $this->assertInstanceOf('\Google\Protobuf\Internal\Descriptor', $fieldDesc->getMessageType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());

        // Oneof int field
        $fieldDesc = $desc->getField(5);
        $this->assertSame('oneof_int32', $fieldDesc->getName());
        $this->assertSame(51, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::OPTIONAL, $fieldDesc->getLabel());
        $this->assertSame(GPBType::INT32, $fieldDesc->getType());
        $this->assertFalse($fieldDesc->isMap());
        $this->assertSame(0, $fieldDesc->getOneofIndex());
        $this->assertInstanceOf('\Google\Protobuf\Internal\OneofDescriptor', $desc->getOneofDecl($fieldDesc->getOneofIndex()));

        // Map int-enum field
        $fieldDesc = $desc->getField(6);
        $this->assertSame('map_int32_enum', $fieldDesc->getName());
        $this->assertSame(71, $fieldDesc->getNumber());
        $this->assertSame(GPBLabel::REPEATED, $fieldDesc->getLabel());
        $this->assertSame(GPBType::MESSAGE, $fieldDesc->getType());
        $this->assertTrue($fieldDesc->isMap());
        $mapDesc = $fieldDesc->getMessageType();
        $this->assertSame('google.protobuf.map_entry', $mapDesc->getFullName());
        $this->assert(GPBType::INT32, $mapDesc->getField(1)->getType());
        $this->assert(GPBType::ENUM, $mapDesc->getField(2)->getType());
        $this->assertSame(-1, $fieldDesc->getOneofIndex());
    }

    /**
     * @expectedException \Exception
     */
    public function testFieldDescriptorEnumException()
    {
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class(new TestDescriptorsMessage()));
        $fieldDesc = $desc->getField(0);
        $fieldDesc->getEnumType();
    }

    /**
     * @expectedException \Exception
     */
    public function testFieldDescriptorMessageException()
    {
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
        $fieldDesc = $desc->getField(5);
        $oneofDesc = $desc->getOneofDecl($fieldDesc->getOneofIndex());

        $this->assertSame('my_oneof', $oneofDesc->getName());
        $fieldDescFromOneof = $oneofDesc->getField(0);
        $this->assertSame($fieldDesc, $fieldDescFromOneof);
        $this->assertSame(1, $oneofDesc->getFieldCount());
    }
}
