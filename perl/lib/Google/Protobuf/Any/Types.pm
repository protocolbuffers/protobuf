package Google::Protobuf::Any::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'Any',
    as InstanceOf['Google::Protobuf::Any::Any'];

coerce 'Any',
    from HashRef, via { 'Google::Protobuf::Any::Any'->new($_) };

declare 'RepeatedAny',
    as ArrayRef[Any()];

coerce 'RepeatedAny',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Any::Any'->new($_) } @$_ ] };

declare 'MapStringAny',
    as HashRef[Any()];

1;
