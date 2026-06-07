package Google::Protobuf::Struct::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'NullValue',
    as (Int | Str);

declare 'Struct',
    as InstanceOf['Google::Protobuf::Struct::Struct'];

coerce 'Struct',
    from HashRef, via { 'Google::Protobuf::Struct::Struct'->new($_) };

declare 'RepeatedStruct',
    as ArrayRef[Struct()];

coerce 'RepeatedStruct',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Struct::Struct'->new($_) } @$_ ] };

declare 'MapStringStruct',
    as HashRef[Struct()];

declare 'FieldsEntry',
    as InstanceOf['Google::Protobuf::Struct::Struct::FieldsEntry'];

coerce 'FieldsEntry',
    from HashRef, via { 'Google::Protobuf::Struct::Struct::FieldsEntry'->new($_) };

declare 'RepeatedFieldsEntry',
    as ArrayRef[FieldsEntry()];

coerce 'RepeatedFieldsEntry',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Struct::Struct::FieldsEntry'->new($_) } @$_ ] };

declare 'MapStringFieldsEntry',
    as HashRef[FieldsEntry()];

declare 'Value',
    as InstanceOf['Google::Protobuf::Struct::Value'];

coerce 'Value',
    from HashRef, via { 'Google::Protobuf::Struct::Value'->new($_) };

declare 'RepeatedValue',
    as ArrayRef[Value()];

coerce 'RepeatedValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Struct::Value'->new($_) } @$_ ] };

declare 'MapStringValue',
    as HashRef[Value()];

declare 'ListValue',
    as InstanceOf['Google::Protobuf::Struct::ListValue'];

coerce 'ListValue',
    from HashRef, via { 'Google::Protobuf::Struct::ListValue'->new($_) };

declare 'RepeatedListValue',
    as ArrayRef[ListValue()];

coerce 'RepeatedListValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Struct::ListValue'->new($_) } @$_ ] };

declare 'MapStringListValue',
    as HashRef[ListValue()];

1;
