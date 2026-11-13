<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

require_once('test_base.php');

use Closed_enum_required\EditionsClosedEnum;
use Closed_enum_required\Proto2ClosedEnum;
use Closed_enum_required\TestEditionsClosedEnumAndRequired;
use Closed_enum_required\TestProto2ClosedEnumAndRequired;
use Google\Protobuf\DescriptorPool;

class ClosedEnumAndRequiredTest extends TestBase
{
    public function testProto2ClosedEnumTreatedAsOpenOnWire(): void
    {
        $m = new TestProto2ClosedEnumAndRequired();
        // Default value of Proto2ClosedEnum is first declared constant (PROTO2_FIRST = 1)
        $this->assertFalse($m->hasOptionalEnum());
        $this->assertSame(Proto2ClosedEnum::PROTO2_FIRST, $m->getOptionalEnum());
        $this->assertFalse($m->hasOneofEnum());
        $this->assertSame(Proto2ClosedEnum::PROTO2_FIRST, $m->getOneofEnum());

        // Wire bytes: field 3 varint (tag = (3 << 3) | 0 = 0x18) with unknown value 99 (0x63),
        // plus field 4 varint (tag = (4 << 3) | 0 = 0x20) with known 2 and unknown 77 (0x4d).
        $wire = "\x18\x63\x20\x02\x20\x4d";
        $m->mergeFromString($wire);

        // Because PHP treats all enums as open (matching Ruby), unknown enum value 99
        // is retained on the field rather than routed to the unknown field set.
        $this->assertTrue($m->hasOptionalEnum());
        $this->assertSame(99, $m->getOptionalEnum());
        $this->assertSame([2, 77], iterator_to_array($m->getRepeatedEnum()));

        // Discarding unknown fields must NOT lose the unknown enum values on the fields!
        $m->discardUnknownFields();
        $this->assertTrue($m->hasOptionalEnum());
        $this->assertSame(99, $m->getOptionalEnum());
        $this->assertSame([2, 77], iterator_to_array($m->getRepeatedEnum()));

        // Re-serializing preserves the values on their fields
        $roundTrip = new TestProto2ClosedEnumAndRequired();
        $roundTrip->mergeFromString($m->serializeToString());
        $this->assertSame(99, $roundTrip->getOptionalEnum());
        $this->assertSame([2, 77], iterator_to_array($roundTrip->getRepeatedEnum()));
    }

    public function testEditionsClosedEnumTreatedAsOpenOnWire(): void
    {
        $m = new TestEditionsClosedEnumAndRequired();
        $this->assertFalse($m->hasOptionalEnum());
        $this->assertSame(EditionsClosedEnum::EDITIONS_FIRST, $m->getOptionalEnum());

        // Field 3 = 123 (0x18 0x7b), Field 4 packed [1, 124] (0x22 0x02 0x01 0x7c)
        $m->mergeFromString("\x18\x7b\x22\x02\x01\x7c");
        $this->assertTrue($m->hasOptionalEnum());
        $this->assertSame(123, $m->getOptionalEnum());
        $this->assertSame([1, 124], iterator_to_array($m->getRepeatedEnum()));

        $m->discardUnknownFields();
        $this->assertSame(123, $m->getOptionalEnum());
        $this->assertSame([1, 124], iterator_to_array($m->getRepeatedEnum()));
    }

    public function testProto2AndEditionsRequiredFieldsTreatedAsExplicitPresence(): void
    {
        new TestProto2ClosedEnumAndRequired();
        new TestEditionsClosedEnumAndRequired();
        $pool = DescriptorPool::getGeneratedPool();

        $p2Desc = $pool->getDescriptorByClassName(TestProto2ClosedEnumAndRequired::class);
        $this->assertTrue($p2Desc->getField(0)->isRequired());
        $this->assertTrue($p2Desc->getField(0)->hasPresence());

        $edDesc = $pool->getDescriptorByClassName(TestEditionsClosedEnumAndRequired::class);
        $this->assertTrue($edDesc->getField(0)->isRequired());
        $this->assertTrue($edDesc->getField(0)->hasPresence());

        // Serializing and decoding with unset required fields succeeds without runtime check
        $p2 = new TestProto2ClosedEnumAndRequired();
        $this->assertFalse($p2->hasRequiredString());
        $this->assertFalse($p2->hasRequiredInt32());
        $this->assertSame('', $p2->serializeToString());

        $p2->setRequiredString('');
        $p2->setRequiredInt32(0);
        $this->assertTrue($p2->hasRequiredString());
        $this->assertTrue($p2->hasRequiredInt32());

        $decoded = new TestProto2ClosedEnumAndRequired();
        $decoded->mergeFromString($p2->serializeToString());
        $this->assertTrue($decoded->hasRequiredString());
        $this->assertTrue($decoded->hasRequiredInt32());
        $this->assertSame('', $decoded->getRequiredString());
        $this->assertSame(0, $decoded->getRequiredInt32());

        $decoded->clearRequiredInt32();
        $this->assertFalse($decoded->hasRequiredInt32());
    }
}
