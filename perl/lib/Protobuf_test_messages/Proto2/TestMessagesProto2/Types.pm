package Protobuf_test_messages::Proto2::TestMessagesProto2::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'ForeignEnumProto2',
    as (Int | Str);

declare 'TestAllTypesProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2'];

coerce 'TestAllTypesProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2'->new($_) };

declare 'RepeatedTestAllTypesProto2',
    as ArrayRef[TestAllTypesProto2()];

coerce 'RepeatedTestAllTypesProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2'->new($_) } @$_ ] };

declare 'MapStringTestAllTypesProto2',
    as HashRef[TestAllTypesProto2()];

declare 'NestedEnum',
    as (Int | Str);

declare 'NestedMessage',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage'];

coerce 'NestedMessage',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage'->new($_) };

declare 'RepeatedNestedMessage',
    as ArrayRef[NestedMessage()];

coerce 'RepeatedNestedMessage',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage'->new($_) } @$_ ] };

declare 'MapStringNestedMessage',
    as HashRef[NestedMessage()];

declare 'MapInt32Int32Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32Int32Entry'];

coerce 'MapInt32Int32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32Int32Entry'->new($_) };

declare 'RepeatedMapInt32Int32Entry',
    as ArrayRef[MapInt32Int32Entry()];

coerce 'RepeatedMapInt32Int32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32Int32Entry'->new($_) } @$_ ] };

declare 'MapStringMapInt32Int32Entry',
    as HashRef[MapInt32Int32Entry()];

declare 'MapInt64Int64Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt64Int64Entry'];

coerce 'MapInt64Int64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt64Int64Entry'->new($_) };

declare 'RepeatedMapInt64Int64Entry',
    as ArrayRef[MapInt64Int64Entry()];

coerce 'RepeatedMapInt64Int64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt64Int64Entry'->new($_) } @$_ ] };

declare 'MapStringMapInt64Int64Entry',
    as HashRef[MapInt64Int64Entry()];

declare 'MapUint32Uint32Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint32Uint32Entry'];

coerce 'MapUint32Uint32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint32Uint32Entry'->new($_) };

declare 'RepeatedMapUint32Uint32Entry',
    as ArrayRef[MapUint32Uint32Entry()];

coerce 'RepeatedMapUint32Uint32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint32Uint32Entry'->new($_) } @$_ ] };

declare 'MapStringMapUint32Uint32Entry',
    as HashRef[MapUint32Uint32Entry()];

declare 'MapUint64Uint64Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint64Uint64Entry'];

coerce 'MapUint64Uint64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint64Uint64Entry'->new($_) };

declare 'RepeatedMapUint64Uint64Entry',
    as ArrayRef[MapUint64Uint64Entry()];

coerce 'RepeatedMapUint64Uint64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapUint64Uint64Entry'->new($_) } @$_ ] };

declare 'MapStringMapUint64Uint64Entry',
    as HashRef[MapUint64Uint64Entry()];

declare 'MapSint32Sint32Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint32Sint32Entry'];

coerce 'MapSint32Sint32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint32Sint32Entry'->new($_) };

declare 'RepeatedMapSint32Sint32Entry',
    as ArrayRef[MapSint32Sint32Entry()];

coerce 'RepeatedMapSint32Sint32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint32Sint32Entry'->new($_) } @$_ ] };

declare 'MapStringMapSint32Sint32Entry',
    as HashRef[MapSint32Sint32Entry()];

declare 'MapSint64Sint64Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint64Sint64Entry'];

coerce 'MapSint64Sint64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint64Sint64Entry'->new($_) };

declare 'RepeatedMapSint64Sint64Entry',
    as ArrayRef[MapSint64Sint64Entry()];

coerce 'RepeatedMapSint64Sint64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSint64Sint64Entry'->new($_) } @$_ ] };

declare 'MapStringMapSint64Sint64Entry',
    as HashRef[MapSint64Sint64Entry()];

declare 'MapFixed32Fixed32Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed32Fixed32Entry'];

coerce 'MapFixed32Fixed32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed32Fixed32Entry'->new($_) };

declare 'RepeatedMapFixed32Fixed32Entry',
    as ArrayRef[MapFixed32Fixed32Entry()];

coerce 'RepeatedMapFixed32Fixed32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed32Fixed32Entry'->new($_) } @$_ ] };

declare 'MapStringMapFixed32Fixed32Entry',
    as HashRef[MapFixed32Fixed32Entry()];

declare 'MapFixed64Fixed64Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed64Fixed64Entry'];

coerce 'MapFixed64Fixed64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed64Fixed64Entry'->new($_) };

declare 'RepeatedMapFixed64Fixed64Entry',
    as ArrayRef[MapFixed64Fixed64Entry()];

coerce 'RepeatedMapFixed64Fixed64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapFixed64Fixed64Entry'->new($_) } @$_ ] };

declare 'MapStringMapFixed64Fixed64Entry',
    as HashRef[MapFixed64Fixed64Entry()];

declare 'MapSfixed32Sfixed32Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed32Sfixed32Entry'];

coerce 'MapSfixed32Sfixed32Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed32Sfixed32Entry'->new($_) };

declare 'RepeatedMapSfixed32Sfixed32Entry',
    as ArrayRef[MapSfixed32Sfixed32Entry()];

coerce 'RepeatedMapSfixed32Sfixed32Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed32Sfixed32Entry'->new($_) } @$_ ] };

declare 'MapStringMapSfixed32Sfixed32Entry',
    as HashRef[MapSfixed32Sfixed32Entry()];

declare 'MapSfixed64Sfixed64Entry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed64Sfixed64Entry'];

coerce 'MapSfixed64Sfixed64Entry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed64Sfixed64Entry'->new($_) };

declare 'RepeatedMapSfixed64Sfixed64Entry',
    as ArrayRef[MapSfixed64Sfixed64Entry()];

coerce 'RepeatedMapSfixed64Sfixed64Entry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapSfixed64Sfixed64Entry'->new($_) } @$_ ] };

declare 'MapStringMapSfixed64Sfixed64Entry',
    as HashRef[MapSfixed64Sfixed64Entry()];

declare 'MapInt32BoolEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32BoolEntry'];

coerce 'MapInt32BoolEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32BoolEntry'->new($_) };

declare 'RepeatedMapInt32BoolEntry',
    as ArrayRef[MapInt32BoolEntry()];

coerce 'RepeatedMapInt32BoolEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32BoolEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32BoolEntry',
    as HashRef[MapInt32BoolEntry()];

declare 'MapInt32FloatEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32FloatEntry'];

coerce 'MapInt32FloatEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32FloatEntry'->new($_) };

declare 'RepeatedMapInt32FloatEntry',
    as ArrayRef[MapInt32FloatEntry()];

coerce 'RepeatedMapInt32FloatEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32FloatEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32FloatEntry',
    as HashRef[MapInt32FloatEntry()];

declare 'MapInt32DoubleEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32DoubleEntry'];

coerce 'MapInt32DoubleEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32DoubleEntry'->new($_) };

declare 'RepeatedMapInt32DoubleEntry',
    as ArrayRef[MapInt32DoubleEntry()];

coerce 'RepeatedMapInt32DoubleEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32DoubleEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32DoubleEntry',
    as HashRef[MapInt32DoubleEntry()];

declare 'MapInt32NestedMessageEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32NestedMessageEntry'];

coerce 'MapInt32NestedMessageEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32NestedMessageEntry'->new($_) };

declare 'RepeatedMapInt32NestedMessageEntry',
    as ArrayRef[MapInt32NestedMessageEntry()];

coerce 'RepeatedMapInt32NestedMessageEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapInt32NestedMessageEntry'->new($_) } @$_ ] };

declare 'MapStringMapInt32NestedMessageEntry',
    as HashRef[MapInt32NestedMessageEntry()];

declare 'MapBoolBoolEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapBoolBoolEntry'];

coerce 'MapBoolBoolEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapBoolBoolEntry'->new($_) };

declare 'RepeatedMapBoolBoolEntry',
    as ArrayRef[MapBoolBoolEntry()];

coerce 'RepeatedMapBoolBoolEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapBoolBoolEntry'->new($_) } @$_ ] };

declare 'MapStringMapBoolBoolEntry',
    as HashRef[MapBoolBoolEntry()];

declare 'MapStringStringEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringStringEntry'];

coerce 'MapStringStringEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringStringEntry'->new($_) };

declare 'RepeatedMapStringStringEntry',
    as ArrayRef[MapStringStringEntry()];

coerce 'RepeatedMapStringStringEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringStringEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringStringEntry',
    as HashRef[MapStringStringEntry()];

declare 'MapStringBytesEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringBytesEntry'];

coerce 'MapStringBytesEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringBytesEntry'->new($_) };

declare 'RepeatedMapStringBytesEntry',
    as ArrayRef[MapStringBytesEntry()];

coerce 'RepeatedMapStringBytesEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringBytesEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringBytesEntry',
    as HashRef[MapStringBytesEntry()];

declare 'MapStringNestedMessageEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedMessageEntry'];

coerce 'MapStringNestedMessageEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedMessageEntry'->new($_) };

declare 'RepeatedMapStringNestedMessageEntry',
    as ArrayRef[MapStringNestedMessageEntry()];

coerce 'RepeatedMapStringNestedMessageEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedMessageEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringNestedMessageEntry',
    as HashRef[MapStringNestedMessageEntry()];

declare 'MapStringForeignMessageEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignMessageEntry'];

coerce 'MapStringForeignMessageEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignMessageEntry'->new($_) };

declare 'RepeatedMapStringForeignMessageEntry',
    as ArrayRef[MapStringForeignMessageEntry()];

coerce 'RepeatedMapStringForeignMessageEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignMessageEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringForeignMessageEntry',
    as HashRef[MapStringForeignMessageEntry()];

declare 'MapStringNestedEnumEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedEnumEntry'];

coerce 'MapStringNestedEnumEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedEnumEntry'->new($_) };

declare 'RepeatedMapStringNestedEnumEntry',
    as ArrayRef[MapStringNestedEnumEntry()];

coerce 'RepeatedMapStringNestedEnumEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringNestedEnumEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringNestedEnumEntry',
    as HashRef[MapStringNestedEnumEntry()];

declare 'MapStringForeignEnumEntry',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignEnumEntry'];

coerce 'MapStringForeignEnumEntry',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignEnumEntry'->new($_) };

declare 'RepeatedMapStringForeignEnumEntry',
    as ArrayRef[MapStringForeignEnumEntry()];

coerce 'RepeatedMapStringForeignEnumEntry',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MapStringForeignEnumEntry'->new($_) } @$_ ] };

declare 'MapStringMapStringForeignEnumEntry',
    as HashRef[MapStringForeignEnumEntry()];

declare 'Data',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::Data'];

coerce 'Data',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::Data'->new($_) };

declare 'RepeatedData',
    as ArrayRef[Data()];

coerce 'RepeatedData',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::Data'->new($_) } @$_ ] };

declare 'MapStringData',
    as HashRef[Data()];

declare 'MultiWordGroupField',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MultiWordGroupField'];

coerce 'MultiWordGroupField',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MultiWordGroupField'->new($_) };

declare 'RepeatedMultiWordGroupField',
    as ArrayRef[MultiWordGroupField()];

coerce 'RepeatedMultiWordGroupField',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MultiWordGroupField'->new($_) } @$_ ] };

declare 'MapStringMultiWordGroupField',
    as HashRef[MultiWordGroupField()];

declare 'MessageSetCorrect',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrect'];

coerce 'MessageSetCorrect',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrect'->new($_) };

declare 'RepeatedMessageSetCorrect',
    as ArrayRef[MessageSetCorrect()];

coerce 'RepeatedMessageSetCorrect',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrect'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrect',
    as HashRef[MessageSetCorrect()];

declare 'MessageSetCorrectExtension1',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension1'];

coerce 'MessageSetCorrectExtension1',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension1'->new($_) };

declare 'RepeatedMessageSetCorrectExtension1',
    as ArrayRef[MessageSetCorrectExtension1()];

coerce 'RepeatedMessageSetCorrectExtension1',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension1'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrectExtension1',
    as HashRef[MessageSetCorrectExtension1()];

declare 'MessageSetCorrectExtension2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension2'];

coerce 'MessageSetCorrectExtension2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension2'->new($_) };

declare 'RepeatedMessageSetCorrectExtension2',
    as ArrayRef[MessageSetCorrectExtension2()];

coerce 'RepeatedMessageSetCorrectExtension2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::MessageSetCorrectExtension2'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrectExtension2',
    as HashRef[MessageSetCorrectExtension2()];

declare 'ExtensionWithOneof',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::ExtensionWithOneof'];

coerce 'ExtensionWithOneof',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::ExtensionWithOneof'->new($_) };

declare 'RepeatedExtensionWithOneof',
    as ArrayRef[ExtensionWithOneof()];

coerce 'RepeatedExtensionWithOneof',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::ExtensionWithOneof'->new($_) } @$_ ] };

declare 'MapStringExtensionWithOneof',
    as HashRef[ExtensionWithOneof()];

declare 'ForeignMessageProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::ForeignMessageProto2'];

coerce 'ForeignMessageProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::ForeignMessageProto2'->new($_) };

declare 'RepeatedForeignMessageProto2',
    as ArrayRef[ForeignMessageProto2()];

coerce 'RepeatedForeignMessageProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::ForeignMessageProto2'->new($_) } @$_ ] };

declare 'MapStringForeignMessageProto2',
    as HashRef[ForeignMessageProto2()];

declare 'GroupField',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::GroupField'];

coerce 'GroupField',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::GroupField'->new($_) };

declare 'RepeatedGroupField',
    as ArrayRef[GroupField()];

coerce 'RepeatedGroupField',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::GroupField'->new($_) } @$_ ] };

declare 'MapStringGroupField',
    as HashRef[GroupField()];

declare 'UnknownToTestAllTypes',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes'];

coerce 'UnknownToTestAllTypes',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes'->new($_) };

declare 'RepeatedUnknownToTestAllTypes',
    as ArrayRef[UnknownToTestAllTypes()];

coerce 'RepeatedUnknownToTestAllTypes',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes'->new($_) } @$_ ] };

declare 'MapStringUnknownToTestAllTypes',
    as HashRef[UnknownToTestAllTypes()];

declare 'OptionalGroup',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes::OptionalGroup'];

coerce 'OptionalGroup',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes::OptionalGroup'->new($_) };

declare 'RepeatedOptionalGroup',
    as ArrayRef[OptionalGroup()];

coerce 'RepeatedOptionalGroup',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::UnknownToTestAllTypes::OptionalGroup'->new($_) } @$_ ] };

declare 'MapStringOptionalGroup',
    as HashRef[OptionalGroup()];

declare 'NullHypothesisProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::NullHypothesisProto2'];

coerce 'NullHypothesisProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::NullHypothesisProto2'->new($_) };

declare 'RepeatedNullHypothesisProto2',
    as ArrayRef[NullHypothesisProto2()];

coerce 'RepeatedNullHypothesisProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::NullHypothesisProto2'->new($_) } @$_ ] };

declare 'MapStringNullHypothesisProto2',
    as HashRef[NullHypothesisProto2()];

declare 'EnumOnlyProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::EnumOnlyProto2'];

coerce 'EnumOnlyProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::EnumOnlyProto2'->new($_) };

declare 'RepeatedEnumOnlyProto2',
    as ArrayRef[EnumOnlyProto2()];

coerce 'RepeatedEnumOnlyProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::EnumOnlyProto2'->new($_) } @$_ ] };

declare 'MapStringEnumOnlyProto2',
    as HashRef[EnumOnlyProto2()];

declare 'Bool',
    as (Int | Str);

declare 'OneStringProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::OneStringProto2'];

coerce 'OneStringProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::OneStringProto2'->new($_) };

declare 'RepeatedOneStringProto2',
    as ArrayRef[OneStringProto2()];

coerce 'RepeatedOneStringProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::OneStringProto2'->new($_) } @$_ ] };

declare 'MapStringOneStringProto2',
    as HashRef[OneStringProto2()];

declare 'ProtoWithKeywords',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::ProtoWithKeywords'];

coerce 'ProtoWithKeywords',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::ProtoWithKeywords'->new($_) };

declare 'RepeatedProtoWithKeywords',
    as ArrayRef[ProtoWithKeywords()];

coerce 'RepeatedProtoWithKeywords',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::ProtoWithKeywords'->new($_) } @$_ ] };

declare 'MapStringProtoWithKeywords',
    as HashRef[ProtoWithKeywords()];

declare 'TestAllRequiredTypesProto2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2'];

coerce 'TestAllRequiredTypesProto2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2'->new($_) };

declare 'RepeatedTestAllRequiredTypesProto2',
    as ArrayRef[TestAllRequiredTypesProto2()];

coerce 'RepeatedTestAllRequiredTypesProto2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2'->new($_) } @$_ ] };

declare 'MapStringTestAllRequiredTypesProto2',
    as HashRef[TestAllRequiredTypesProto2()];

declare 'NestedEnum',
    as (Int | Str);

declare 'NestedMessage',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::NestedMessage'];

coerce 'NestedMessage',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::NestedMessage'->new($_) };

declare 'RepeatedNestedMessage',
    as ArrayRef[NestedMessage()];

coerce 'RepeatedNestedMessage',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::NestedMessage'->new($_) } @$_ ] };

declare 'MapStringNestedMessage',
    as HashRef[NestedMessage()];

declare 'Data',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::Data'];

coerce 'Data',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::Data'->new($_) };

declare 'RepeatedData',
    as ArrayRef[Data()];

coerce 'RepeatedData',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::Data'->new($_) } @$_ ] };

declare 'MapStringData',
    as HashRef[Data()];

declare 'MessageSetCorrect',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrect'];

coerce 'MessageSetCorrect',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrect'->new($_) };

declare 'RepeatedMessageSetCorrect',
    as ArrayRef[MessageSetCorrect()];

coerce 'RepeatedMessageSetCorrect',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrect'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrect',
    as HashRef[MessageSetCorrect()];

declare 'MessageSetCorrectExtension1',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension1'];

coerce 'MessageSetCorrectExtension1',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension1'->new($_) };

declare 'RepeatedMessageSetCorrectExtension1',
    as ArrayRef[MessageSetCorrectExtension1()];

coerce 'RepeatedMessageSetCorrectExtension1',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension1'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrectExtension1',
    as HashRef[MessageSetCorrectExtension1()];

declare 'MessageSetCorrectExtension2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension2'];

coerce 'MessageSetCorrectExtension2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension2'->new($_) };

declare 'RepeatedMessageSetCorrectExtension2',
    as ArrayRef[MessageSetCorrectExtension2()];

coerce 'RepeatedMessageSetCorrectExtension2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllRequiredTypesProto2::MessageSetCorrectExtension2'->new($_) } @$_ ] };

declare 'MapStringMessageSetCorrectExtension2',
    as HashRef[MessageSetCorrectExtension2()];

declare 'TestLargeOneof',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof'];

coerce 'TestLargeOneof',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof'->new($_) };

declare 'RepeatedTestLargeOneof',
    as ArrayRef[TestLargeOneof()];

coerce 'RepeatedTestLargeOneof',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof'->new($_) } @$_ ] };

declare 'MapStringTestLargeOneof',
    as HashRef[TestLargeOneof()];

declare 'A1',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A1'];

coerce 'A1',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A1'->new($_) };

declare 'RepeatedA1',
    as ArrayRef[A1()];

coerce 'RepeatedA1',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A1'->new($_) } @$_ ] };

declare 'MapStringA1',
    as HashRef[A1()];

declare 'A2',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A2'];

coerce 'A2',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A2'->new($_) };

declare 'RepeatedA2',
    as ArrayRef[A2()];

coerce 'RepeatedA2',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A2'->new($_) } @$_ ] };

declare 'MapStringA2',
    as HashRef[A2()];

declare 'A3',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A3'];

coerce 'A3',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A3'->new($_) };

declare 'RepeatedA3',
    as ArrayRef[A3()];

coerce 'RepeatedA3',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A3'->new($_) } @$_ ] };

declare 'MapStringA3',
    as HashRef[A3()];

declare 'A4',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A4'];

coerce 'A4',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A4'->new($_) };

declare 'RepeatedA4',
    as ArrayRef[A4()];

coerce 'RepeatedA4',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A4'->new($_) } @$_ ] };

declare 'MapStringA4',
    as HashRef[A4()];

declare 'A5',
    as InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A5'];

coerce 'A5',
    from HashRef, via { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A5'->new($_) };

declare 'RepeatedA5',
    as ArrayRef[A5()];

coerce 'RepeatedA5',
    from ArrayRef[HashRef], via { [ map { 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestLargeOneof::A5'->new($_) } @$_ ] };

declare 'MapStringA5',
    as HashRef[A5()];

1;
