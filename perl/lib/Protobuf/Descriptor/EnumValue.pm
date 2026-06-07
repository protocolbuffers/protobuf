package Protobuf::Descriptor::EnumValue;

use Moo;
use strict;
require Protobuf;
use warnings;

our $VERSION = '0.05';


sub name {
    my ($self) = @_;
    return _xs_name($self);
}

sub number {
    my ($self) = @_;
    return _xs_number($self);
}

sub index {
    my ($self) = @_;
    return _xs_index($self);
}

__PACKAGE__->meta->make_immutable;

1;
