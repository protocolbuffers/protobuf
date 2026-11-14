<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\DescriptorPool;
use Google\Protobuf\DescriptorProto;
use Google\Protobuf\DescriptorProto\ExtensionRange;
use Google\Protobuf\DescriptorProto\ReservedRange;
use Google\Protobuf\Edition;
use Google\Protobuf\EnumDescriptorProto;
use Google\Protobuf\EnumValueDescriptorProto;
use Google\Protobuf\FeatureSet;
use Google\Protobuf\FeatureSet\FieldPresence;
use Google\Protobuf\FieldDescriptorProto;
use Google\Protobuf\FieldDescriptorProto\Label;
use Google\Protobuf\FieldDescriptorProto\Type;
use Google\Protobuf\FieldOptions;
use Google\Protobuf\FileDescriptorProto;
use Google\Protobuf\MessageOptions;
use Google\Protobuf\OneofDescriptorProto;
use Google\Protobuf\UninterpretedOption;
use Google\Protobuf\UninterpretedOption\NamePart;
use Test_descriptor_proto\TestProtoSchema;

class PublicDescriptorProtoTest extends TestBase
{
    private function buildSampleDescriptorProto(): DescriptorProto
    {
        $idField = new FieldDescriptorProto([
            'name' => 'row_id',
            'number' => 1,
            'label' => Label::LABEL_REQUIRED,
            'type' => Type::TYPE_INT64,
        ]);

        $tagsField = new FieldDescriptorProto([
            'name' => 'tags',
            'number' => 2,
            'label' => Label::LABEL_REPEATED,
            'type' => Type::TYPE_INT32,
            'options' => new FieldOptions(['packed' => true]),
        ]);

        $oneofField = new FieldDescriptorProto([
            'name' => 'note',
            'number' => 3,
            'label' => Label::LABEL_OPTIONAL,
            'type' => Type::TYPE_STRING,
            'oneof_index' => 0,
        ]);

        $nestedMsg = new DescriptorProto([
            'name' => 'NestedEntry',
            'field' => [
                new FieldDescriptorProto([
                    'name' => 'value',
                    'number' => 1,
                    'label' => Label::LABEL_OPTIONAL,
                    'type' => Type::TYPE_STRING,
                ]),
            ],
        ]);

        $nestedEnum = new EnumDescriptorProto([
            'name' => 'RowState',
            'value' => [
                new EnumValueDescriptorProto(['name' => 'ACTIVE', 'number' => 1]),
                new EnumValueDescriptorProto(['name' => 'ARCHIVED', 'number' => 2]),
            ],
        ]);

        $uninterpreted = new UninterpretedOption([
            'name' => [
                new NamePart([
                    'name_part' => 'custom_opt',
                    'is_extension' => true,
                ]),
            ],
            'positive_int_value' => 42,
        ]);

        $msgOptions = new MessageOptions([
            'deprecated' => false,
            'features' => new FeatureSet([
                'field_presence' => FieldPresence::EXPLICIT,
            ]),
            'uninterpreted_option' => [$uninterpreted],
        ]);

        return new DescriptorProto([
            'name' => 'CustomerRow',
            'field' => [$idField, $tagsField, $oneofField],
            'nested_type' => [$nestedMsg],
            'enum_type' => [$nestedEnum],
            'oneof_decl' => [new OneofDescriptorProto(['name' => 'payload'])],
            'extension_range' => [new ExtensionRange(['start' => 100, 'end' => 200])],
            'reserved_range' => [new ReservedRange(['start' => 50, 'end' => 60])],
            'reserved_name' => ['legacy_col'],
            'options' => $msgOptions,
        ]);
    }

    public function testPublicDescriptorProtoBinaryAndJsonRoundtrip(): void
    {
        $proto = $this->buildSampleDescriptorProto();
        $this->assertTrue($proto->hasName());
        $this->assertSame('CustomerRow', $proto->getName());

        $bytes = $proto->serializeToString();
        $this->assertNotEmpty($bytes);

        $decoded = new DescriptorProto();
        $this->assertFalse($decoded->hasName());
        $decoded->mergeFromString($bytes);

        $this->assertTrue($decoded->hasName());
        $this->assertSame('CustomerRow', $decoded->getName());
        $this->assertCount(3, $decoded->getField());
        $this->assertSame('row_id', $decoded->getField()[0]->getName());
        $this->assertSame(Label::LABEL_REQUIRED, $decoded->getField()[0]->getLabel());
        $this->assertSame(Type::TYPE_INT64, $decoded->getField()[0]->getType());
        $this->assertTrue($decoded->getField()[1]->getOptions()->hasPacked());
        $this->assertTrue($decoded->getField()[1]->getOptions()->getPacked());
        $this->assertTrue($decoded->getField()[2]->hasOneofIndex());
        $this->assertSame(0, $decoded->getField()[2]->getOneofIndex());
        $this->assertSame('NestedEntry', $decoded->getNestedType()[0]->getName());
        $this->assertSame('RowState', $decoded->getEnumType()[0]->getName());
        $this->assertSame('payload', $decoded->getOneofDecl()[0]->getName());
        $this->assertSame(100, $decoded->getExtensionRange()[0]->getStart());
        $this->assertSame(50, $decoded->getReservedRange()[0]->getStart());
        $this->assertSame('legacy_col', $decoded->getReservedName()[0]);
        $this->assertSame(
            FieldPresence::EXPLICIT,
            $decoded->getOptions()->getFeatures()->getFieldPresence()
        );
        $this->assertSame(
            'custom_opt',
            $decoded->getOptions()->getUninterpretedOption()[0]->getName()[0]->getNamePart()
        );

        // JSON roundtrip (including proto2 required NamePart fields and closed enums).
        $json = $decoded->serializeToJsonString();
        $fromJson = new DescriptorProto();
        $fromJson->mergeFromJsonString($json);
        $this->assertEquals($decoded, $fromJson);

        // Explicit presence clear() works on proto2 DescriptorProto fields.
        $fromJson->clearName();
        $this->assertFalse($fromJson->hasName());
        $this->assertSame('', $fromJson->getName());
    }

    public function testProtoSchemaWrapperRoundtrip(): void
    {
        // Mirrors BigQueryStorage ProtoSchema wrapping a public DescriptorProto.
        $schema = new TestProtoSchema();
        $this->assertFalse($schema->hasProtoDescriptor());

        $rowDesc = $this->buildSampleDescriptorProto();
        $fileDesc = new FileDescriptorProto([
            'name' => 'customer_row.proto',
            'package' => 'analytics',
            'syntax' => 'editions',
            'edition' => Edition::EDITION_2023,
            'message_type' => [$rowDesc],
        ]);

        $schema->setProtoDescriptor($rowDesc);
        $schema->setFileDescriptor($fileDesc);
        $this->assertTrue($schema->hasProtoDescriptor());
        $this->assertTrue($schema->hasFileDescriptor());

        $wire = $schema->serializeToString();
        $decoded = new TestProtoSchema();
        $decoded->mergeFromString($wire);

        $this->assertTrue($decoded->hasProtoDescriptor());
        $this->assertSame('CustomerRow', $decoded->getProtoDescriptor()->getName());
        $this->assertCount(3, $decoded->getProtoDescriptor()->getField());
        $this->assertSame(Edition::EDITION_2023, $decoded->getFileDescriptor()->getEdition());
        $this->assertSame('CustomerRow', $decoded->getFileDescriptor()->getMessageType()[0]->getName());
    }

    public function testDescriptorPoolLookupForPublicDescriptorProto(): void
    {
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(DescriptorProto::class);
        $this->assertNotNull($desc);
        $this->assertSame('google.protobuf.DescriptorProto', $desc->getFullName());
        $this->assertSame(DescriptorProto::class, $desc->getClass());
    }

    public function testPublicAndInternalDescriptorProtosCoexistInPool(): void
    {
        if (extension_loaded('protobuf')) {
            $this->markTestSkipped('Internal bootstrap descriptor classes are only used by the pure-PHP runtime.');
        }

        $publicProto = new DescriptorProto(['name' => 'PublicMsg']);
        $internalProto = new \Google\Protobuf\Internal\DescriptorProto(['name' => 'InternalMsg']);

        $this->assertSame('PublicMsg', $publicProto->getName());
        $this->assertSame('InternalMsg', $internalProto->getName());
    }
}
