=encoding UTF-8

=head1 NAME

Protobuf::WKT::Timestamp - Mixin for google.protobuf.Timestamp

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # In your .proto
    import "google/protobuf/timestamp.proto";

    message MyLog {
      google.protobuf.Timestamp event_time = 1;
    }

    # In your Perl code
    use Time::Piece;

    my $log = MyLog->new;
    my $timestamp = $log->event_time;

    # Setting from Time::Piece
    my $now = localtime;
    $timestamp->from_time_piece($now);

    # Getting a Time::Piece object
    my $tp = $timestamp->to_time_piece();
    print $tp->strftime("%Y-%m-%d %H:%M:%S"), "
";

    # Getting ISO 8601 string
    my $iso_string = $timestamp->to_iso8601(); # e.g., "2026-04-01T12:00:00Z"

=head1 DESCRIPTION

This module provides helper methods for the generated class corresponding to the C<google.protobuf.Timestamp> Well-Known Type. These methods are injected into the C<Protobuf::WKT::Timestamp> class, which should be automatically used when C<google/protobuf/timestamp.proto> is processed.

The C<Timestamp> type represents a point in time independent of any time zone or calendar, encoded as seconds and nanoseconds since the Unix epoch.

=head1 METHODS

=head2 to_time_piece()

Converts the timestamp to a L<Time::Piece> object. Note that C<nanos> are not fully supported by L<Time::Piece>, so precision may be lost.

=head2 from_time_piece($tp)

Sets the C<seconds> and C<nanos> fields from a L<Time::Piece> object C<$tp>. C<nanos> will be set to 0.

Returns C<$self> for chaining.

=head2 to_iso8601()

Returns an ISO 8601 string representation of the timestamp in UTC (e.g., "2026-04-01T12:00:00Z"). This uses L<Time::Piece> internally, so nanosecond precision is lost.

=head2 get_injected_methods()

Internal method used by L<Protobuf::ClassGenerator> to list methods to inject into the class.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Message>, L<Time::Piece>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::WKT::Timestamp;

use strict;
use warnings;
use Time::Piece;

sub to_time_piece {
    my ($self) = @_;
    my $seconds = $self->seconds;
    return Time::Piece->gmtime($seconds);
}

sub from_time_piece {
    my ($self, $tp) = @_;
    $self->seconds($tp->epoch);
    $self->nanos(0); # Time::Piece doesn't support nanos well
    return $self;
}

sub to_iso8601 {
    my ($self) = @_;
    my $tp = $self->to_time_piece();
    return $tp->datetime . 'Z';
}

sub get_injected_methods {
    return qw(to_time_piece from_time_piece to_iso8601);
}

1;
