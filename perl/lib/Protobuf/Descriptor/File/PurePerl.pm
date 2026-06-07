package Protobuf::Descriptor::File::PurePerl;

use parent 'Protobuf::Descriptor::Base::PurePerl', 'Protobuf::Descriptor::File';
use strict;
use warnings;

our $VERSION = '0.05';

sub name {
    my ($self) = @_;
    return $self->{_data}{name};
}

sub package {
    my ($self) = @_;
    return $self->{_data}{package};
}

sub get_package { shift->package(@_) }

sub message_type_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{message_types} || []};
}

sub top_level_message_count { shift->message_type_count(@_) }

sub get_message_type {
    my ($self, $index) = @_;
    return $self->{_data}{message_types}[$index];
}

sub get_top_level_message { shift->get_message_type(@_) }

sub enum_type_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{enum_types} || []};
}

sub top_level_enum_count { shift->enum_type_count(@_) }

sub get_enum_type {
    my ($self, $index) = @_;
    return $self->{_data}{enum_types}[$index];
}

sub get_top_level_enum { shift->get_enum_type(@_) }

1;
