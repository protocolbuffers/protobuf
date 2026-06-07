package Protobuf::Types::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'Types',
    as InstanceOf['Protobuf::Types::Types'];

coerce 'Types',
    from HashRef, via { 'Protobuf::Types::Types'->new($_) };

declare 'Enum',
    as Int;

declare 'Message',
    as InstanceOf['Protobuf::Types::Types::Message'];

coerce 'Message',
    from HashRef, via { 'Protobuf::Types::Types::Message'->new($_) };

1;
