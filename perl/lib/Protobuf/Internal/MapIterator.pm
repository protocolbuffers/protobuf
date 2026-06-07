=encoding UTF-8

=head1 NAME

Protobuf::Internal::MapIterator - Internal XS implementation for map iterators

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # This module is not intended for direct use.
    # It is used internally by Protobuf::Internal::Map.

=head1 DESCRIPTION

This module provides the internal XS implementation for iterating over Protobuf map fields, interfacing with the C<upb_MapIterator> structure.

=head1 METHODS

=head2 next_key()

Returns the next key in the map iteration.

=head2 next_value()

Returns the next value in the map iteration.

=cut

package Protobuf::Internal::MapIterator;

use strict;
require Protobuf;
use warnings;

our $VERSION = '0.05';


sub next_key {
    my ($self) = @_;
    return $self->_xs_next_key();
}

sub next_value {
    my ($self) = @_;
    return $self->_xs_next_value();
}

1;
