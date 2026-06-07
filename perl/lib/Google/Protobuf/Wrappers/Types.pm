package Google::Protobuf::Wrappers::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'DoubleValue',
    as InstanceOf['Google::Protobuf::Wrappers::DoubleValue'];

coerce 'DoubleValue',
    from HashRef, via { 'Google::Protobuf::Wrappers::DoubleValue'->new($_) };

declare 'RepeatedDoubleValue',
    as ArrayRef[DoubleValue()];

coerce 'RepeatedDoubleValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::DoubleValue'->new($_) } @$_ ] };

declare 'MapStringDoubleValue',
    as HashRef[DoubleValue()];

declare 'FloatValue',
    as InstanceOf['Google::Protobuf::Wrappers::FloatValue'];

coerce 'FloatValue',
    from HashRef, via { 'Google::Protobuf::Wrappers::FloatValue'->new($_) };

declare 'RepeatedFloatValue',
    as ArrayRef[FloatValue()];

coerce 'RepeatedFloatValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::FloatValue'->new($_) } @$_ ] };

declare 'MapStringFloatValue',
    as HashRef[FloatValue()];

declare 'Int64Value',
    as InstanceOf['Google::Protobuf::Wrappers::Int64Value'];

coerce 'Int64Value',
    from HashRef, via { 'Google::Protobuf::Wrappers::Int64Value'->new($_) };

declare 'RepeatedInt64Value',
    as ArrayRef[Int64Value()];

coerce 'RepeatedInt64Value',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::Int64Value'->new($_) } @$_ ] };

declare 'MapStringInt64Value',
    as HashRef[Int64Value()];

declare 'UInt64Value',
    as InstanceOf['Google::Protobuf::Wrappers::UInt64Value'];

coerce 'UInt64Value',
    from HashRef, via { 'Google::Protobuf::Wrappers::UInt64Value'->new($_) };

declare 'RepeatedUInt64Value',
    as ArrayRef[UInt64Value()];

coerce 'RepeatedUInt64Value',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::UInt64Value'->new($_) } @$_ ] };

declare 'MapStringUInt64Value',
    as HashRef[UInt64Value()];

declare 'Int32Value',
    as InstanceOf['Google::Protobuf::Wrappers::Int32Value'];

coerce 'Int32Value',
    from HashRef, via { 'Google::Protobuf::Wrappers::Int32Value'->new($_) };

declare 'RepeatedInt32Value',
    as ArrayRef[Int32Value()];

coerce 'RepeatedInt32Value',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::Int32Value'->new($_) } @$_ ] };

declare 'MapStringInt32Value',
    as HashRef[Int32Value()];

declare 'UInt32Value',
    as InstanceOf['Google::Protobuf::Wrappers::UInt32Value'];

coerce 'UInt32Value',
    from HashRef, via { 'Google::Protobuf::Wrappers::UInt32Value'->new($_) };

declare 'RepeatedUInt32Value',
    as ArrayRef[UInt32Value()];

coerce 'RepeatedUInt32Value',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::UInt32Value'->new($_) } @$_ ] };

declare 'MapStringUInt32Value',
    as HashRef[UInt32Value()];

declare 'BoolValue',
    as InstanceOf['Google::Protobuf::Wrappers::BoolValue'];

coerce 'BoolValue',
    from HashRef, via { 'Google::Protobuf::Wrappers::BoolValue'->new($_) };

declare 'RepeatedBoolValue',
    as ArrayRef[BoolValue()];

coerce 'RepeatedBoolValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::BoolValue'->new($_) } @$_ ] };

declare 'MapStringBoolValue',
    as HashRef[BoolValue()];

declare 'StringValue',
    as InstanceOf['Google::Protobuf::Wrappers::StringValue'];

coerce 'StringValue',
    from HashRef, via { 'Google::Protobuf::Wrappers::StringValue'->new($_) };

declare 'RepeatedStringValue',
    as ArrayRef[StringValue()];

coerce 'RepeatedStringValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::StringValue'->new($_) } @$_ ] };

declare 'MapStringStringValue',
    as HashRef[StringValue()];

declare 'BytesValue',
    as InstanceOf['Google::Protobuf::Wrappers::BytesValue'];

coerce 'BytesValue',
    from HashRef, via { 'Google::Protobuf::Wrappers::BytesValue'->new($_) };

declare 'RepeatedBytesValue',
    as ArrayRef[BytesValue()];

coerce 'RepeatedBytesValue',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Wrappers::BytesValue'->new($_) } @$_ ] };

declare 'MapStringBytesValue',
    as HashRef[BytesValue()];

1;
