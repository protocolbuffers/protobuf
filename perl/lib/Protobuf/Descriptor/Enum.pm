=encoding UTF-8

=head1 NAME

Protobuf::Descriptor::Enum - Descriptor for a Protocol Buffer enum

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $enum_def = $pool->find_enum_by_name('my.package.MyEnum');

    if ($enum_def) {
        print "Enum Name: ", $enum_def->full_name, "
";
        for my $i (0 .. $enum_def->value_count - 1) {
            # Note: methods to get value details are on EnumValueDef
            # This example just shows count and name by index.
             print "  Value: ", $enum_def->value_name($i), "
";
        }
    }

=head1 DESCRIPTION

This class represents the descriptor for a single Protocol Buffer enum type. It provides methods to introspect the enum's name and values.

Instances of this class are usually obtained from a L<Protobuf::DescriptorPool>.

=head1 METHODS

=head2 full_name()

Returns the fully qualified name of the enum (e.g., C<my.package.MyEnum>).

=head2 name()

Returns the short name of the enum (e.g., C<MyEnum>).

=head2 value_count()

Returns the number of values defined in this enum.

=head2 value_name($index)

Returns the string name of the enum value at the given C<$index> (0-based).

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Descriptor>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Descriptor::Enum;

use Moo;
use strict;
require Protobuf;
use warnings;

our $VERSION = '0.05';


sub full_name {
    my ($self) = @_;
    return _xs_full_name($self);
}

sub name {
    my ($self) = @_;
    return _xs_name($self);
}

sub value_count {
    my ($self) = @_;
    return _xs_value_count($self);
}

sub value_name {
    my ($self, $index) = @_;
    return _xs_value_name($self, $index);
}

sub get_value {
    my ($self, $index) = @_;
    return _xs_get_value($self, $index);
}

__PACKAGE__->meta->make_immutable;

1;
