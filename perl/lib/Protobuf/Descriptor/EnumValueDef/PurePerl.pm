package Protobuf::Descriptor::EnumValueDef::PurePerl;

use parent 'Protobuf::Descriptor::Base::PurePerl';
use strict;
use warnings;

our $VERSION = '0.05';

sub name {
    my ($self) = @_;
    return $self->{_data}{name};
}

sub number {
    my ($self) = @_;
    return $self->{_data}{number};
}

1;
