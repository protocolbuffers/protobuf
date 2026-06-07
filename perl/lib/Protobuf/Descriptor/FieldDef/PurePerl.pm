package Protobuf::Descriptor::FieldDef::PurePerl;

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

sub type {
    my ($self) = @_;
    my $type = $self->{_data}{type};
    my %types = (
        1 => 'double', 2 => 'float', 3 => 'int64', 4 => 'uint64',
        5 => 'int32', 6 => 'fixed64', 7 => 'fixed32', 8 => 'bool',
        9 => 'string', 10 => 'group', 11 => 'message', 12 => 'bytes',
        13 => 'uint32', 14 => 'enum', 15 => 'sfixed32', 16 => 'sfixed64',
        17 => 'sint32', 18 => 'sint64',
    );
    return $types{$type} || 'unknown';
}

sub type_number {
    my ($self) = @_;
    return $self->{_data}{type};
}

sub label {
    my ($self) = @_;
    my $label = $self->{_data}{label};
    my %labels = (
        1 => 'optional', 2 => 'required', 3 => 'repeated',
    );
    return $labels{$label} || 'unknown';
}

sub label_number {
    my ($self) = @_;
    return $self->{_data}{label};
}

sub is_extension {
    my ($self) = @_;
    return $self->{_data}{is_extension} || 0;
}

sub is_packed {
    my ($self) = @_;
    return $self->{_data}{is_packed} || 0;
}

sub is_repeated {
    my ($self) = @_;
    return 1 if ($self->label_number || 0) == 3;
    return 0;
}

sub is_required {
    my ($self) = @_;
    return 1 if ($self->label_number || 0) == 2;
    return 0;
}

sub is_map {
    my ($self) = @_;
    return 0 unless ($self->label_number || 0) == 3; # REPEATED
    return 0 unless ($self->type_number || 0) == 11; # MESSAGE
    my $subm = $self->message_type;
    return 0 unless $subm;
    return $subm->is_map_entry;
}

sub is_submessage {
    my ($self) = @_;
    return 1 if ($self->type_number || 0) == 11;
    return 0;
}

sub message_type {
    my ($self) = @_;
    return $self->{_data}{message_type};
}

sub enum_type {
    my ($self) = @_;
    return $self->{_data}{enum_type};
}

sub containing_oneof {
    my ($self) = @_;
    return $self->{_data}{containing_oneof};
}

1;
