=encoding UTF-8

=head1 NAME

Protobuf::WKT - Well-Known Type Registry

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    use Protobuf::WKT;

    my $class = Protobuf::WKT->get_extension_class('google.protobuf.Timestamp');
    # $class is 'Protobuf::WKT::Timestamp'

=head1 DESCRIPTION

This module provides a registry for mapping the fully qualified names of Protocol Buffer Well-Known Types (WKTs) to their specific Perl implementation classes under the C<Protobuf::WKT::> namespace.

It's primarily used internally by L<Protobuf::ClassGenerator> to ensure that messages like C<google.protobuf.Any> or C<google.protobuf.Timestamp> are blessed into classes that provide additional helper methods (e.g., for time conversions or C<Any> packing/unpacking).

=head1 METHODS

=head2 get_extension_class($full_name)

Given the full name of a Well-Known Type (e.g., C<google.protobuf.Any>), returns the corresponding Perl class name (e.g., C<Protobuf::WKT::Any>), or C<undef> if the name is not a registered WKT.

=head2 get_all_wkts()

Returns a list of all registered Well-Known Type full names.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::ClassGenerator>, and the various modules in the C<Protobuf::WKT::> namespace.

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::WKT;

use strict;
use warnings;

my %REGISTRY = (
    'google.protobuf.Any'       => 'Protobuf::WKT::Any',
    'google.protobuf.Timestamp' => 'Protobuf::WKT::Timestamp',
    'google.protobuf.Duration'  => 'Protobuf::WKT::Duration',
    'google.protobuf.Struct'    => 'Protobuf::WKT::Struct',
    'google.protobuf.Value'     => 'Protobuf::WKT::Value',
    'google.protobuf.ListValue'  => 'Protobuf::WKT::ListValue',
    'google.protobuf.FieldMask' => 'Protobuf::WKT::FieldMask',
);

sub get_extension_class {
    my ($class, $full_name) = @_;
    $full_name =~ s/^\.//;
    return $REGISTRY{$full_name};
}

sub get_all_wkts {
    return keys %REGISTRY;
}

1;
