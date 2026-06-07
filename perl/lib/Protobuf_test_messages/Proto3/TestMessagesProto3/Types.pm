package Protobuf_test_messages::Proto3::TestMessagesProto3::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'ForeignEnum',
    as (Int | Str);

declare 'TestAllTypesProto3',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3'];

coerce 'TestAllTypesProto3',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3'->new($_) };

declare 'RepeatedTestAllTypesProto3',
    as ArrayRef[TestAllTypesProto3()];

coerce 'RepeatedTestAllTypesProto3',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3'->new($_) } @$_ ] };

declare 'MapStringTestAllTypesProto3',
    as HashRef[TestAllTypesProto3()];

declare 'NestedEnum',
    as (Int | Str);

declare 'AliasedEnum',
    as (Int | Str);

declare 'NestedMessage',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::NestedMessage'];

coerce 'NestedMessage',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::NestedMessage'->new($_) };

declare 'RepeatedNestedMessage',
    as ArrayRef[NestedMessage()];

coerce 'RepeatedNestedMessage',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::NestedMessage'->new($_) } @$_ ] };

declare 'MapStringNestedMessage',
    as HashRef[NestedMessage()];

declare 'MapInt32Int32Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32Int32Entry'];

coerce 'MapInt32Int32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32Int32Entry'->new($_) };

declare 'RepeatedMapInt32Int32Entry',
    as ArrayRef[MapInt32Int32Entry()];

coerce 'RepeatedMapInt32Int32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32Int32Entry'->new($_) } @$_ ] };

declare 'MapStringMapInt32Int32Entry',
    as HashRef[MapInt32Int32Entry()];

declare 'MapInt64Int64Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt64Int64Entry'];

coerce 'MapInt64Int64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt64Int64Entry'->new($_) };

declare 'RepeatedMapInt64Int64Entry',
    as ArrayRef[MapInt64Int64Entry()];

coerce 'RepeatedMapInt64Int64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt64Int64Entry'->new($_) } @$_ ] };

declare 'MapStringMapInt64Int64Entry',
    as HashRef[MapInt64Int64Entry()];

declare 'MapUint32Uint32Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint32Uint32Entry'];

coerce 'MapUint32Uint32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint32Uint32Entry'->new($_) };

declare 'RepeatedMapUint32Uint32Entry',
    as ArrayRef[MapUint32Uint32Entry()];

coerce 'RepeatedMapUint32Uint32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint32Uint32Entry'->new($_) } @$_ ] };

declare 'MapStringMapUint32Uint32Entry',
    as HashRef[MapUint32Uint32Entry()];

declare 'MapUint64Uint64Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint64Uint64Entry'];

coerce 'MapUint64Uint64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint64Uint64Entry'->new($_) };

declare 'RepeatedMapUint64Uint64Entry',
    as ArrayRef[MapUint64Uint64Entry()];

coerce 'RepeatedMapUint64Uint64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapUint64Uint64Entry'->new($_) } @$_ ] };

declare 'MapStringMapUint64Uint64Entry',
    as HashRef[MapUint64Uint64Entry()];

declare 'MapSint32Sint32Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint32Sint32Entry'];

coerce 'MapSint32Sint32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint32Sint32Entry'->new($_) };

declare 'RepeatedMapSint32Sint32Entry',
    as ArrayRef[MapSint32Sint32Entry()];

coerce 'RepeatedMapSint32Sint32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint32Sint32Entry'->new($_) } @$_ ] };

declare 'MapStringMapSint32Sint32Entry',
    as HashRef[MapSint32Sint32Entry()];

declare 'MapSint64Sint64Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint64Sint64Entry'];

coerce 'MapSint64Sint64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint64Sint64Entry'->new($_) };

declare 'RepeatedMapSint64Sint64Entry',
    as ArrayRef[MapSint64Sint64Entry()];

coerce 'RepeatedMapSint64Sint64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSint64Sint64Entry'->new($_) } @$_ ] };

declare 'MapStringMapSint64Sint64Entry',
    as HashRef[MapSint64Sint64Entry()];

declare 'MapFixed32Fixed32Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed32Fixed32Entry'];

coerce 'MapFixed32Fixed32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed32Fixed32Entry'->new($_) };

declare 'RepeatedMapFixed32Fixed32Entry',
    as ArrayRef[MapFixed32Fixed32Entry()];

coerce 'RepeatedMapFixed32Fixed32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed32Fixed32Entry'->new($_) } @$_ ] };

declare 'MapStringMapFixed32Fixed32Entry',
    as HashRef[MapFixed32Fixed32Entry()];

declare 'MapFixed64Fixed64Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed64Fixed64Entry'];

coerce 'MapFixed64Fixed64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed64Fixed64Entry'->new($_) };

declare 'RepeatedMapFixed64Fixed64Entry',
    as ArrayRef[MapFixed64Fixed64Entry()];

coerce 'RepeatedMapFixed64Fixed64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapFixed64Fixed64Entry'->new($_) } @$_ ] };

declare 'MapStringMapFixed64Fixed64Entry',
    as HashRef[MapFixed64Fixed64Entry()];

declare 'MapSfixed32Sfixed32Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed32Sfixed32Entry'];

coerce 'MapSfixed32Sfixed32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed32Sfixed32Entry'->new($_) };

declare 'RepeatedMapSfixed32Sfixed32Entry',
    as ArrayRef[MapSfixed32Sfixed32Entry()];

coerce 'RepeatedMapSfixed32Sfixed32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed32Sfixed32Entry'->new($_) } @$_ ] };

declare 'MapStringMapSfixed32Sfixed32Entry',
    as HashRef[MapSfixed32Sfixed32Entry()];

declare 'MapSfixed64Sfixed64Entry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed64Sfixed64Entry'];

coerce 'MapSfixed64Sfixed64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed64Sfixed64Entry'->new($_) };

declare 'RepeatedMapSfixed64Sfixed64Entry',
    as ArrayRef[MapSfixed64Sfixed64Entry()];

coerce 'RepeatedMapSfixed64Sfixed64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapSfixed64Sfixed64Entry'->new($_) } @$_ ] };

declare 'MapStringMapSfixed64Sfixed64Entry',
    as HashRef[MapSfixed64Sfixed64Entry()];

declare 'MapInt32FloatEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32FloatEntry'];

coerce 'MapInt32FloatEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32FloatEntry'->new($_) };

declare 'RepeatedMapInt32FloatEntry',
    as ArrayRef[MapInt32FloatEntry()];

coerce 'RepeatedMapInt32FloatEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32FloatEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32FloatEntry',
    as HashRef[MapInt32FloatEntry()];

declare 'MapInt32DoubleEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32DoubleEntry'];

coerce 'MapInt32DoubleEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32DoubleEntry'->new($_) };

declare 'RepeatedMapInt32DoubleEntry',
    as ArrayRef[MapInt32DoubleEntry()];

coerce 'RepeatedMapInt32DoubleEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapInt32DoubleEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32DoubleEntry',
    as HashRef[MapInt32DoubleEntry()];

declare 'MapBoolBoolEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapBoolBoolEntry'];

coerce 'MapBoolBoolEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapBoolBoolEntry'->new($_) };

declare 'RepeatedMapBoolBoolEntry',
    as ArrayRef[MapBoolBoolEntry()];

coerce 'RepeatedMapBoolBoolEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapBoolBoolEntry'->new($_) } @$_ ] };

declare 'MapStringMapBoolBoolEntry',
    as HashRef[MapBoolBoolEntry()];

declare 'MapStringStringEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringStringEntry'];

coerce 'MapStringStringEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringStringEntry'->new($_) };

declare 'RepeatedMapStringStringEntry',
    as ArrayRef[MapStringStringEntry()];

coerce 'RepeatedMapStringStringEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringStringEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringStringEntry',
    as HashRef[MapStringStringEntry()];

declare 'MapStringBytesEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringBytesEntry'];

coerce 'MapStringBytesEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringBytesEntry'->new($_) };

declare 'RepeatedMapStringBytesEntry',
    as ArrayRef[MapStringBytesEntry()];

coerce 'RepeatedMapStringBytesEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringBytesEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringBytesEntry',
    as HashRef[MapStringBytesEntry()];

declare 'MapStringNestedMessageEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedMessageEntry'];

coerce 'MapStringNestedMessageEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedMessageEntry'->new($_) };

declare 'RepeatedMapStringNestedMessageEntry',
    as ArrayRef[MapStringNestedMessageEntry()];

coerce 'RepeatedMapStringNestedMessageEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedMessageEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringNestedMessageEntry',
    as HashRef[MapStringNestedMessageEntry()];

declare 'MapStringForeignMessageEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignMessageEntry'];

coerce 'MapStringForeignMessageEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignMessageEntry'->new($_) };

declare 'RepeatedMapStringForeignMessageEntry',
    as ArrayRef[MapStringForeignMessageEntry()];

coerce 'RepeatedMapStringForeignMessageEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignMessageEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringForeignMessageEntry',
    as HashRef[MapStringForeignMessageEntry()];

declare 'MapStringNestedEnumEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedEnumEntry'];

coerce 'MapStringNestedEnumEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedEnumEntry'->new($_) };

declare 'RepeatedMapStringNestedEnumEntry',
    as ArrayRef[MapStringNestedEnumEntry()];

coerce 'RepeatedMapStringNestedEnumEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringNestedEnumEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringNestedEnumEntry',
    as HashRef[MapStringNestedEnumEntry()];

declare 'MapStringForeignEnumEntry',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignEnumEntry'];

coerce 'MapStringForeignEnumEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignEnumEntry'->new($_) };

declare 'RepeatedMapStringForeignEnumEntry',
    as ArrayRef[MapStringForeignEnumEntry()];

coerce 'RepeatedMapStringForeignEnumEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3::MapStringForeignEnumEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringForeignEnumEntry',
    as HashRef[MapStringForeignEnumEntry()];

declare 'ForeignMessage',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::ForeignMessage'];

coerce 'ForeignMessage',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::ForeignMessage'->new($_) };

declare 'RepeatedForeignMessage',
    as ArrayRef[ForeignMessage()];

coerce 'RepeatedForeignMessage',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::ForeignMessage'->new($_) } @$_ ] };

declare 'MapStringForeignMessage',
    as HashRef[ForeignMessage()];

declare 'NullHypothesisProto3',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::NullHypothesisProto3'];

coerce 'NullHypothesisProto3',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::NullHypothesisProto3'->new($_) };

declare 'RepeatedNullHypothesisProto3',
    as ArrayRef[NullHypothesisProto3()];

coerce 'RepeatedNullHypothesisProto3',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::NullHypothesisProto3'->new($_) } @$_ ] };

declare 'MapStringNullHypothesisProto3',
    as HashRef[NullHypothesisProto3()];

declare 'EnumOnlyProto3',
    as InstanceOf['Protobuf_test_messages::Proto3::TestMessagesProto3::EnumOnlyProto3'];

coerce 'EnumOnlyProto3',
    from HashRef, via { 'Protobuf_test_messages::Proto3::TestMessagesProto3::EnumOnlyProto3'->new($_) };

declare 'RepeatedEnumOnlyProto3',
    as ArrayRef[EnumOnlyProto3()];

coerce 'RepeatedEnumOnlyProto3',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto3::TestMessagesProto3::EnumOnlyProto3'->new($_) } @$_ ] };

declare 'MapStringEnumOnlyProto3',
    as HashRef[EnumOnlyProto3()];

declare 'Bool',
    as (Int | Str);

1;
