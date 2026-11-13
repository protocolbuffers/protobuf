<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

require_once('test_base.php');

use Google\Protobuf\DescriptorPool;
use Google\Protobuf\Internal\DescriptorPool as InternalDescriptorPool;
use Google\Protobuf\Internal\DescriptorProto;
use Google\Protobuf\Internal\FieldDescriptorProto;
use Google\Protobuf\Internal\FieldDescriptorProto\Label;
use Google\Protobuf\Internal\FieldDescriptorProto\Type;
use Google\Protobuf\Internal\FileDescriptor;
use Google\Protobuf\Internal\FileDescriptorProto;
use Google\Protobuf\Internal\FileDescriptorSet;
use Presence_packing_editions\EditionsOpenEnum;
use Presence_packing_editions\TestEditionsImplicitAndExpandedFile;
use Presence_packing_editions\TestEditionsPresencePacking;
use Presence_packing_editions\TestEditionsPresencePacking\NestedMessage;
use Presence_packing_proto2\TestProto2PresencePacking;
use Presence_packing_proto3\TestProto3PresencePacking;

class PresenceAndPackingTest extends TestBase
{
    private function buildFieldMap(string $className): array
    {
        new $className();
        $pool = DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName($className);
        $map = [];
        for ($i = 0; $i < $desc->getFieldCount(); $i++) {
            $field = $desc->getField($i);
            $map[$field->getName()] = $field;
        }
        return $map;
    }

    public function testProto2FieldPresenceMetadata(): void
    {
        $fields = $this->buildFieldMap(TestProto2PresencePacking::class);

        $this->assertTrue($fields['optional_int32']->hasPresence());
        $this->assertTrue($fields['optional_int64']->hasPresence());
        $this->assertTrue($fields['optional_bool']->hasPresence());
        $this->assertTrue($fields['optional_string']->hasPresence());
        $this->assertTrue($fields['default_int32']->hasPresence());
        $this->assertFalse($fields['repeated_default']->hasPresence());
    }

    public function testEditionsFeatureInheritanceAndOverrides(): void
    {
        $fields = $this->buildFieldMap(TestEditionsPresencePacking::class);

        $this->assertTrue($fields['explicit_int32']->hasPresence());
        $this->assertFalse($fields['explicit_int32']->isRequired());

        $this->assertFalse($fields['implicit_int32']->hasPresence());
        $this->assertFalse($fields['implicit_int32']->isRequired());

        $this->assertTrue($fields['explicit_enum']->hasPresence());
        $this->assertTrue($fields['oneof_int32']->hasPresence());
        $this->assertTrue($fields['nested']->hasPresence());
        $this->assertFalse($fields['repeated_default']->hasPresence());

        $nestedFields = $this->buildFieldMap(NestedMessage::class);
        $this->assertTrue($nestedFields['inherited_explicit_int32']->hasPresence());
        $this->assertFalse($nestedFields['overridden_implicit_int32']->hasPresence());

        $fileOverriddenFields = $this->buildFieldMap(TestEditionsImplicitAndExpandedFile::class);
        $this->assertFalse($fileOverriddenFields['inherited_implicit_int32']->hasPresence());
        $this->assertTrue($fileOverriddenFields['overridden_explicit_int32']->hasPresence());
    }

    public function testProto2RepeatedPackingWireFormat(): void
    {
        // Tag for field 10: wire type 0 (varint) = (10 << 3) | 0 = 0x50
        // Tag for field 11: wire type 2 (length-delimited) = (11 << 3) | 2 = 0x5a
        // Tag for field 12: wire type 0 (varint) = (12 << 3) | 0 = 0x60
        // Tag for field 13: wire type 0 (varint) = (13 << 3) | 0 = 0x68
        $m = new TestProto2PresencePacking();
        $m->setRepeatedDefault([1, 2]);
        $this->assertSame("\x50\x01\x50\x02", $m->serializeToString());

        $m = new TestProto2PresencePacking();
        $m->setRepeatedPacked([1, 2]);
        $this->assertSame("\x5a\x02\x01\x02", $m->serializeToString());

        $m = new TestProto2PresencePacking();
        $m->setRepeatedUnpacked([1, 2]);
        $this->assertSame("\x60\x01\x60\x02", $m->serializeToString());

        $m = new TestProto2PresencePacking();
        @$m->setRepeatedDeprecated([1, 2]);
        $this->assertSame("\x68\x01\x68\x02", $m->serializeToString());
    }

    public function testProto3RepeatedPackingWithUnrelatedOption(): void
    {
        // Field 10: packed -> (10 << 3) | 2 = 0x52, len 2, [1, 2]
        // Field 11: [packed = false] -> (11 << 3) | 0 = 0x58
        // Field 12: [deprecated = true] -> MUST remain packed: (12 << 3) | 2 = 0x62
        $m = new TestProto3PresencePacking();
        $m->setRepeatedDefault([1, 2]);
        $this->assertSame("\x52\x02\x01\x02", $m->serializeToString());

        $m = new TestProto3PresencePacking();
        $m->setRepeatedUnpacked([1, 2]);
        $this->assertSame("\x58\x01\x58\x02", $m->serializeToString());

        $m = new TestProto3PresencePacking();
        @$m->setRepeatedDeprecated([1, 2]);
        $this->assertSame("\x62\x02\x01\x02", $m->serializeToString());
    }

    public function testEditionsRepeatedPackingWireFormat(): void
    {
        // Field 10: default packed -> (10 << 3) | 2 = 0x52
        // Field 11: EXPANDED -> (11 << 3) | 0 = 0x58
        // Field 12: [deprecated = true] -> packed -> (12 << 3) | 2 = 0x62
        $m = new TestEditionsPresencePacking();
        $m->setRepeatedDefault([1, 2]);
        $this->assertSame("\x52\x02\x01\x02", $m->serializeToString());

        $m = new TestEditionsPresencePacking();
        $m->setRepeatedExpanded([1, 2]);
        $this->assertSame("\x58\x01\x58\x02", $m->serializeToString());

        $m = new TestEditionsPresencePacking();
        @$m->setRepeatedDeprecated([1, 2]);
        $this->assertSame("\x62\x02\x01\x02", $m->serializeToString());

        // File-level EXPANDED with field-level PACKED override, plus nested message inheritance
        $fileOverridden = new TestEditionsImplicitAndExpandedFile();
        $fileOverridden->setInheritedExpandedInt32([1, 2]); // (3 << 3) | 0 = 0x18
        $this->assertSame("\x18\x01\x18\x02", $fileOverridden->serializeToString());

        $fileOverridden = new TestEditionsImplicitAndExpandedFile();
        $fileOverridden->setOverriddenPackedInt32([1, 2]); // (4 << 3) | 2 = 0x22
        $this->assertSame("\x22\x02\x01\x02", $fileOverridden->serializeToString());

        $nested = new NestedMessage();
        $nested->setInheritedPackedInt32([1, 2]); // (3 << 3) | 2 = 0x1a
        $this->assertSame("\x1a\x02\x01\x02", $nested->serializeToString());

        $nested = new NestedMessage();
        $nested->setOverriddenExpandedInt32([1, 2]); // (4 << 3) | 0 = 0x20
        $this->assertSame("\x20\x01\x20\x02", $nested->serializeToString());
    }

    public function testExplicitZeroSerializes(): void
    {
        $m = new TestProto2PresencePacking();
        $this->assertFalse($m->hasOptionalInt32());
        $this->assertFalse($m->hasDefaultInt32());
        $this->assertSame(42, $m->getDefaultInt32());
        $this->assertSame('', $m->serializeToString());

        // Setting explicit zero marks presence and serializes on the wire
        $m->setOptionalInt32(0);
        $m->setOptionalInt64(0);
        $m->setOptionalBool(false);
        $m->setOptionalString('');
        $this->assertTrue($m->hasOptionalInt32());
        $this->assertTrue($m->hasOptionalInt64());
        $this->assertTrue($m->hasOptionalBool());
        $this->assertTrue($m->hasOptionalString());
        $wire = $m->serializeToString();
        $this->assertNotSame('', $wire);

        $decoded = new TestProto2PresencePacking();
        $decoded->mergeFromString($wire);
        $this->assertTrue($decoded->hasOptionalInt32());
        $this->assertTrue($decoded->hasOptionalInt64());
        $this->assertTrue($decoded->hasOptionalBool());
        $this->assertTrue($decoded->hasOptionalString());
        $this->assertSame(0, $decoded->getOptionalInt32());
        $this->assertFalse($decoded->getOptionalBool());
        $this->assertSame('', $decoded->getOptionalString());
    }

    #########################################################
    # FileDescriptor::resolveEdition() (pure-PHP only; the C
    # extension resolves syntax inside upb).
    #########################################################

    private function skipIfCExtension(): void
    {
        if (extension_loaded('protobuf')) {
            $this->markTestSkipped('Syntax resolution is handled by upb in the C extension.');
        }
    }

    private function newFileProto(string $name, string $syntax): FileDescriptorProto
    {
        $file = new FileDescriptorProto();
        $file->setName($name);
        if ($syntax !== '') {
            $file->setSyntax($syntax);
        }
        return $file;
    }

    public static function validSyntaxProvider(): array
    {
        // Raw Edition enum numbers: data providers run even when the test is
        // skipped, and Google\Protobuf\Internal\Edition must not be autoloaded
        // under the C extension.
        $proto2 = 998;   // Edition::EDITION_PROTO2
        $proto3 = 999;   // Edition::EDITION_PROTO3
        $e2023 = 1000;   // Edition::EDITION_2023
        return [
            'missing syntax means proto2' => ['', null, $proto2],
            'explicit proto2' => ['proto2', null, $proto2],
            'proto3' => ['proto3', null, $proto3],
            'editions 2023' => ['editions', $e2023, $e2023],
        ];
    }

    /**
     * @dataProvider validSyntaxProvider
     */
    public function testResolveEditionValidSyntax(
        string $syntax,
        ?int $edition,
        int $expected
    ): void {
        $this->skipIfCExtension();
        $file = $this->newFileProto('valid.proto', $syntax);
        if ($edition !== null) {
            $file->setEdition($edition);
        }
        $this->assertSame($expected, FileDescriptor::resolveEdition($file));
    }

    public function testResolveEditionRejectsEditionsWithoutEdition(): void
    {
        $this->skipIfCExtension();
        $file = $this->newFileProto('no_edition.proto', 'editions');

        $this->expectException(\Exception::class);
        $this->expectExceptionMessage(
            'File "no_edition.proto" has syntax="editions", but no edition is specified');
        FileDescriptor::resolveEdition($file);
    }

    public function testResolveEditionRejectsUnknownSyntax(): void
    {
        $this->skipIfCExtension();
        $file = $this->newFileProto('bad.proto', 'proto4');

        $this->expectException(\Exception::class);
        $this->expectExceptionMessage('File "bad.proto" has invalid syntax "proto4"');
        FileDescriptor::resolveEdition($file);
    }

    public function testFileWithoutSyntaxLoadsWithProto2Rules(): void
    {
        $this->skipIfCExtension();

        // Mimics metadata generated by protoc for a proto2 file: no `syntax`.
        $file = $this->newFileProto('syntax_default_proto2.proto', '');
        $file->setPackage('syntax_default_proto2');
        $message = new DescriptorProto();
        $message->setName('SyntaxDefaultMsg');
        $message->setField([
            new FieldDescriptorProto([
                'name' => 'scalar',
                'number' => 1,
                'label' => Label::LABEL_OPTIONAL,
                'type' => Type::TYPE_INT32,
            ]),
            new FieldDescriptorProto([
                'name' => 'numbers',
                'number' => 2,
                'label' => Label::LABEL_REPEATED,
                'type' => Type::TYPE_INT32,
            ]),
        ]);
        $file->setMessageType([$message]);
        $set = new FileDescriptorSet();
        $set->setFile([$file]);

        $pool = InternalDescriptorPool::getGeneratedPool();
        $pool->internalAddGeneratedFile($set->serializeToString());

        $desc = $pool->getDescriptorByProtoName('syntax_default_proto2.SyntaxDefaultMsg');
        $this->assertNotNull($desc);
        $this->assertTrue($desc->getFieldByNumber(1)->hasPresence());
        $this->assertFalse($desc->getFieldByNumber(2)->getPacked());
    }

    public function testFileWithUnknownSyntaxIsRejectedByPool(): void
    {
        $this->skipIfCExtension();

        $file = $this->newFileProto('syntax_invalid.proto', 'proto4');
        $file->setPackage('syntax_invalid');
        $set = new FileDescriptorSet();
        $set->setFile([$file]);

        $this->expectException(\Exception::class);
        $this->expectExceptionMessage('File "syntax_invalid.proto" has invalid syntax "proto4"');
        InternalDescriptorPool::getGeneratedPool()
            ->internalAddGeneratedFile($set->serializeToString());
    }
}
