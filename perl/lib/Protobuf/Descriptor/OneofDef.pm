=encoding UTF-8

=head1 NAME

Protobuf::Descriptor::OneofDef - Descriptor for a Protocol Buffer oneof

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $msg_def = $pool->find_message_by_name('my.package.MyMessage');
    my $oneof_def = $msg_def->find_oneof_by_name('my_oneof');

    if ($oneof_def) {
        print "Oneof Name: ", $oneof_def->name, "
";
        foreach my $field ($oneof_def->fields) {
            print "  Field: ", $field->name, "
";
        }
    }

=head1 DESCRIPTION

This class represents the descriptor for a single C<oneof> definition within a message. A C<oneof> ensures that at most one of a set of fields can be set on a message instance.

Instances of this class are usually obtained from a L<Protobuf::Descriptor::MessageDef>.

=head1 METHODS

=head2 full_name()

Returns the fully qualified name of the oneof.

=head2 name()

Returns the short name of the oneof.

=head2 field_count()

Returns the number of fields belonging to this oneof.

=head2 get_field($index)

Returns the L<Protobuf::Descriptor::Field> at the given C<$index> (0-based) within the oneof.

=head2 fields()

Returns a list of all L<Protobuf::Descriptor::Field> objects belonging to this oneof.

=head2 is_synthetic()

Returns true if this oneof was synthesised for a proto3 optional field.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Descriptor>, L<Protobuf::Descriptor::MessageDef>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Descriptor::OneofDef;

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

sub field_count {
    my ($self) = @_;
    return _xs_field_count($self);
}

sub get_field {
    my ($self, $index) = @_;
    return _xs_field($self, $index);
}

sub fields {
    my ($self) = @_;
    my @fields;
    for (my $i = 0; $i < $self->field_count; $i++) {
        push @fields, $self->get_field($i);
    }
    return @fields;
}

sub is_synthetic {
    my ($self) = @_;
    return _xs_is_synthetic($self);
}

__PACKAGE__->meta->make_immutable;

1;
