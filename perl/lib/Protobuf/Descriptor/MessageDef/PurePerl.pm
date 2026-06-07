package Protobuf::Descriptor::MessageDef::PurePerl;

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

sub perl_class_name {
    my ($self) = @_;
    return $self->{_data}{perl_class} if $self->{_data}{perl_class};
    my $full = $self->full_name;
    $full =~ s/^\.//;
    return join('::', map { ucfirst($_) } split(/\./, $full));
}

sub field_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{fields} || []};
}

sub get_field {
    my ($self, $index) = @_;
    return $self->{_data}{fields}[$index];
}

sub find_field_by_name {
    my ($self, $name) = @_;
    foreach my $field (@{$self->{_data}{fields} || []}) {
        return $field if $field->name eq $name;
    }
    return undef;
}

sub find_field_by_number {
    my ($self, $number) = @_;
    foreach my $field (@{$self->{_data}{fields} || []}) {
        return $field if $field->number == $number;
    }
    return undef;
}

sub oneof_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{oneofs} || []};
}

sub get_oneof {
    my ($self, $index) = @_;
    return $self->{_data}{oneofs}[$index];
}

sub find_oneof_by_name {
    my ($self, $name) = @_;
    foreach my $oneof (@{$self->{_data}{oneofs} || []}) {
        return $oneof if $oneof->name eq $name;
    }
    return undef;
}

sub nested_type_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{nested_types} || []};
}

sub nested_message_count { shift->nested_type_count(@_) }

sub get_nested_type {
    my ($self, $index) = @_;
    return $self->{_data}{nested_types}[$index];
}

sub get_nested_message { shift->get_nested_type(@_) }

sub enum_type_count {
    my ($self) = @_;
    return scalar @{$self->{_data}{enum_types} || []};
}

sub nested_enum_count { shift->enum_type_count(@_) }

sub get_enum_type {
    my ($self, $index) = @_;
    return $self->{_data}{enum_types}[$index];
}

sub get_nested_enum { shift->get_enum_type(@_) }

sub is_map_entry {
    my ($self) = @_;
    return $self->{_data}{options}{map_entry} || 0;
}

1;
