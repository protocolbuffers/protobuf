=encoding UTF-8

=head1 NAME

Protobuf::WKT::Duration - Mixin for google.protobuf.Duration

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # In your .proto
    import "google/protobuf/duration.proto";

    message MyEvent {
      google.protobuf.Duration elapsed = 1;
    }

    # In your Perl code
    my $event = MyEvent->new;
    my $duration = $event->elapsed;

    # Setting from seconds
    $duration->from_seconds(1.5);
    # $duration->seconds is 1, $duration->nanos is 500,000,000

    # Getting total seconds
    my $total_seconds = $duration->to_seconds(); # Returns 1.5

=head1 DESCRIPTION

This module provides helper methods for the generated class corresponding to the C<google.protobuf.Duration> Well-Known Type. These methods are injected into the C<Protobuf::WKT::Duration> class, which should be automatically used when C<google/protobuf/duration.proto> is processed.

The C<Duration> type represents a signed, fixed-length span of time.

=head1 METHODS

=head2 to_seconds()

Returns the total duration in seconds as a floating-point number (seconds + nanos / 1e9).

=head2 from_seconds($seconds)

Sets the C<seconds> and C<nanos> fields from a total number of seconds (can be fractional).

Returns C<$self> for chaining.

=head2 get_injected_methods()

Internal method used by L<Protobuf::ClassGenerator> to list methods to inject into the class.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Message>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::WKT::Duration;

use strict;
use warnings;

sub to_seconds {
    my ($self) = @_;
    return $self->seconds + ($self->nanos / 1_000_000_000);
}

sub from_seconds {
    my ($self, $seconds) = @_;
    my $s = int($seconds);
    my $n = int(($seconds - $s) * 1_000_000_000);
    $self->seconds($s);
    $self->nanos($n);
    return $self;
}

sub get_injected_methods {
    return qw(to_seconds from_seconds);
}

1;
