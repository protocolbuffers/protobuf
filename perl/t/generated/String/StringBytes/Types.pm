package String::StringBytes::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'Bytes',
    as InstanceOf['String::StringBytes::Bytes'];

coerce 'Bytes',
    from HashRef, via { 'String::StringBytes::Bytes'->new($_) };

1;
