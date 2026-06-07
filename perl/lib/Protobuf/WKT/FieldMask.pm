=encoding UTF-8

=head1 NAME

Protobuf::WKT::FieldMask - Mixin for google.protobuf.FieldMask

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # In your .proto
    import "google/protobuf/field_mask.proto";

    message UpdateUserRequest {
      User user = 1;
      google.protobuf.FieldMask update_mask = 2;
    }

    # In your Perl code
    my $req = UpdateUserRequest->new;
    my $mask = $req->update_mask;

    # Setting from string
    $mask->from_string('display_name,email');
    # $mask->paths is now ['display_name', 'email']

    # Getting as string
    my $mask_string = $mask->to_string(); # Returns "display_name,email"

=head1 DESCRIPTION

This module provides helper methods for the generated class corresponding to the C<google.protobuf.FieldMask> Well-Known Type. These methods are injected into the C<Protobuf::WKT::FieldMask> class, which should be automatically used when C<google/protobuf/field_mask.proto> is processed.

The C<FieldMask> type is used to specify a subset of fields that should be returned by a get operation or modified by an update operation.

=head1 METHODS

=head2 to_string()

Returns the field mask paths as a single comma-separated string.

=head2 from_string($str)

Parses a comma-separated string and populates the C<paths> repeated field.

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

package Protobuf::WKT::FieldMask;

use strict;
use warnings;

sub to_string {
    my ($self) = @_;
    return join(',', @{$self->paths});
}

sub from_string {
    my ($self, $str) = @_;
    @{$self->paths} = split(',', $str);
    return $self;
}

sub get_injected_methods {
    return qw(to_string from_string);
}

1;
