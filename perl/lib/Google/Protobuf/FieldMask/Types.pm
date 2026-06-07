package Google::Protobuf::FieldMask::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'FieldMask',
    as InstanceOf['Google::Protobuf::FieldMask::FieldMask'];

coerce 'FieldMask',
    from HashRef, via { 'Google::Protobuf::FieldMask::FieldMask'->new($_) };

declare 'RepeatedFieldMask',
    as ArrayRef[FieldMask()];

coerce 'RepeatedFieldMask',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::FieldMask::FieldMask'->new($_) } @$_ ] };

declare 'MapStringFieldMask',
    as HashRef[FieldMask()];

1;
