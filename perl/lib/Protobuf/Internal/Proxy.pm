=encoding UTF-8

=head1 NAME

Protobuf::Internal::Proxy - Base class for tied XS objects

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    package MyTiedClass;
    use parent 'Protobuf::Internal::Proxy';

    # ... implementation ...

=head1 DESCRIPTION

This module provides common helper functions for classes that tie Perl variables to internal XS objects, for example, delegating method calls to the underlying XS object.

=cut

package Protobuf::Internal::Proxy;

use strict;
use warnings;

# Common delegation logic for tied objects
sub _xs_delegate {
    my ($self, $method, @args) = @_;
    my $xs = ref($self) eq 'HASH' ? $self->{_xs} : $self;
    return $xs->$method(@args);
}

1;
