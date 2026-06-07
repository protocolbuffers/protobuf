package Google::Protobuf::Duration::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'Duration',
    as InstanceOf['Google::Protobuf::Duration::Duration'];

coerce 'Duration',
    from HashRef, via { 'Google::Protobuf::Duration::Duration'->new($_) };

declare 'RepeatedDuration',
    as ArrayRef[Duration()];

coerce 'RepeatedDuration',
    from ArrayRef[HashRef], via { [ map { 'Google::Protobuf::Duration::Duration'->new($_) } @$_ ] };

declare 'MapStringDuration',
    as HashRef[Duration()];

1;
