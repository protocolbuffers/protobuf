package Protobuf::Descriptor::OneofDef::PurePerl;

use parent 'Protobuf::Descriptor::Base::PurePerl';
use strict;
use warnings;

our $VERSION = '0.05';

sub name {
    my ($self) = @_;
    return $self->{_data}{name};
}

sub field_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{fields} || []};
}

sub get_field {
    my ($self, $index) = @_;
    return $self->{_data}{fields}[$index];
}

sub fields {
    my ($self) = @_;
    return @{$self->{_data}{fields} || []};
}

1;
