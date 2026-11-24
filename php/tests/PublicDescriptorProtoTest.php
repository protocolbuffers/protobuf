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
use Descriptors\TestDescriptorsMessage;
use Presence_packing_proto2\TestProto2PresencePacking;
use Presence_packing_editions\TestEditionsPresencePacking;

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

    public function testDescriptorToProtoProto3AndSchemaExport(): void
    {
        new TestDescriptorsMessage();
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(TestDescriptorsMessage::class);
        $this->assertNotNull($desc);

        $proto = $desc->toProto();
        $this->assertInstanceOf(DescriptorProto::class, $proto);
        $this->assertSame('TestDescriptorsMessage', $proto->getName());
        $this->assertCount(8, $proto->getField());
        $this->assertGreaterThanOrEqual(1, count($proto->getNestedType()));
        $this->assertCount(2, $proto->getOneofDecl());
        $this->assertSame('my_oneof', $proto->getOneofDecl()[0]->getName());

        // Verify field metadata (types, numbers, proto3_optional).
        $fieldsByNumber = [];
        foreach ($proto->getField() as $f) {
            $fieldsByNumber[$f->getNumber()] = $f;
        }
        $this->assertSame('optional_int32', $fieldsByNumber[1]->getName());
        $this->assertSame(Type::TYPE_INT32, $fieldsByNumber[1]->getType());
        $this->assertSame(Label::LABEL_OPTIONAL, $fieldsByNumber[1]->getLabel());
        $this->assertSame('repeated_int32', $fieldsByNumber[31]->getName());
        $this->assertSame(Label::LABEL_REPEATED, $fieldsByNumber[31]->getLabel());
        $this->assertTrue($fieldsByNumber[52]->getProto3Optional());

        // End-to-end BigQueryStorage workflow: set $desc->toProto() on ProtoSchema and round-trip.
        $schema = new TestProtoSchema(['proto_descriptor' => $proto]);
        $decodedSchema = new TestProtoSchema();
        $decodedSchema->mergeFromString($schema->serializeToString());
        $this->assertSame('TestDescriptorsMessage', $decodedSchema->getProtoDescriptor()->getName());
        $this->assertCount(8, $decodedSchema->getProtoDescriptor()->getField());
    }

    public function testDescriptorToProtoProto2AndEditions(): void
    {
        new TestProto2PresencePacking();
        new TestEditionsPresencePacking();
        $pool = DescriptorPool::getGeneratedPool();

        // Proto2 descriptor export (custom defaults + packing options).
        $proto2Desc = $pool->getDescriptorByClassName(TestProto2PresencePacking::class);
        $proto2 = $proto2Desc->toProto();
        $this->assertSame('TestProto2PresencePacking', $proto2->getName());
        $proto2Fields = [];
        foreach ($proto2->getField() as $f) {
            $proto2Fields[$f->getName()] = $f;
        }
        $this->assertSame('42', $proto2Fields['default_int32']->getDefaultValue());
        $this->assertSame('custom', $proto2Fields['default_string']->getDefaultValue());
        $this->assertTrue($proto2Fields['repeated_packed']->getOptions()->getPacked());
        $this->assertFalse($proto2Fields['repeated_unpacked']->getOptions()->getPacked());

        // Edition 2023 descriptor export (field features).
        $editionsDesc = $pool->getDescriptorByClassName(TestEditionsPresencePacking::class);
        $editionsProto = $editionsDesc->toProto();
        $this->assertSame('TestEditionsPresencePacking', $editionsProto->getName());
        $editionsFields = [];
        foreach ($editionsProto->getField() as $f) {
            $editionsFields[$f->getName()] = $f;
        }
        $this->assertSame(
            FieldPresence::IMPLICIT,
            $editionsFields['implicit_int32']->getOptions()->getFeatures()->getFieldPresence()
        );
    }

    public function testDescriptorToProtoReturnsFreshCopy(): void
    {
        new TestDescriptorsMessage();
        $desc = DescriptorPool::getGeneratedPool()
            ->getDescriptorByClassName(TestDescriptorsMessage::class);

        $copy1 = $desc->toProto();
        $copy1->setName('MutatedName');
        $copy1->setField([]);

        $copy2 = $desc->toProto();
        $this->assertSame('TestDescriptorsMessage', $copy2->getName());
        $this->assertCount(8, $copy2->getField());
    }

    public function testDescriptorToProtoRejectsPlaceholderClass(): void
    {
        // google/cloud-bigquery-storage <= 2.5.1 declares a placeholder
        // Google\Protobuf\DescriptorProto via Composer "autoload.files". Run in
        // a child process so the placeholder doesn't leak into other tests.
        $script = <<<'PHP'
<?php
namespace Google\Protobuf {
    class DescriptorProto
    {
        public function __construct()
        {
            throw new \LogicException('Proto2 classes are not supported');
        }
    }
}
namespace {
    require $argv[1] . '/vendor/autoload.php';
    new \Descriptors\TestDescriptorsMessage();
    $desc = \Google\Protobuf\DescriptorPool::getGeneratedPool()
        ->getDescriptorByClassName(\Descriptors\TestDescriptorsMessage::class);
    try {
        $desc->toProto();
        echo "NO_EXCEPTION";
    } catch (\Exception $e) {
        echo get_class($e) . ': ' . $e->getMessage();
    }
}
PHP;
        $file = tempnam(sys_get_temp_dir(), 'pb_placeholder_');
        file_put_contents($file, $script);

        $php_root = dirname(__DIR__);
        $cmd = escapeshellarg(PHP_BINARY);
        if (extension_loaded('protobuf')) {
            $cmd .= ' -dextension=' . escapeshellarg(
                $php_root . '/ext/google/protobuf/modules/protobuf.so');
        }
        $cmd .= ' ' . escapeshellarg($file) . ' ' . escapeshellarg($php_root);
        $output = shell_exec($cmd . ' 2>&1');
        unlink($file);

        $this->assertIsString($output);
        $this->assertStringNotContainsString('NO_EXCEPTION', $output);
        $this->assertStringNotContainsString('LogicException', $output);
        $this->assertStringContainsString(
            'Please upgrade google/cloud-bigquery-storage', $output);
    }
}
