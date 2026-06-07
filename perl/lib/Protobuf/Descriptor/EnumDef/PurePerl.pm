package Protobuf::Descriptor::EnumDef::PurePerl;

use parent 'Protobuf::Descriptor::Base::PurePerl';
use strict;
use warnings;

our $VERSION = '0.05';

sub name {
    my ($self) = @_;
    return $self->{_data}{name};
}

sub full_name {
    my ($self) = @_;
    return $self->{_data}{full_name};
}

sub file {
    my ($self) = @_;
    return $self->{_data}{file};
}

sub value_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{values} || []};
}

sub get_value {
    my ($self, $index) = @_;
    return $self->{_data}{values}[$index];
}

sub find_value_by_name {
    my ($self, $name) = @_;
    foreach my $value (@{$self->{_data}{values} || []}) {
        return $value if $value->name eq $name;
    }
    return undef;
}

sub find_value_by_number {
    my ($self, $number) = @_;
    foreach my $value (@{$self->{_data}{values} || []}) {
        return $value if $value->number == $number;
    }
    return undef;
}

1;
