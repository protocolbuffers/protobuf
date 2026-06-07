=encoding UTF-8

=head1 NAME

Protobuf::Descriptor::MessageDef - Descriptor for a Protocol Buffer message

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $msg_def = $pool->find_message_by_name('my.package.MyMessage');

    if ($msg_def) {
        print "Message Name: ", $msg_def->full_name, "
";
        for my $i (0 .. $msg_def->field_count - 1) {
            my $field = $msg_def->get_field($i);
            print "  Field: ", $field->name, " (", $field->type_name, ")
";
        }
    }

=head1 DESCRIPTION

This class represents the descriptor for a single Protocol Buffer message type. It provides methods to introspect the message's structure, including its fields, oneofs, and nested types.

Instances of this class are usually obtained from a L<Protobuf::DescriptorPool>.

=head1 METHODS

=head2 full_name()

Returns the fully qualified name of the message (e.g., C<my.package.MyMessage>).

=head2 name()

Returns the short name of the message (e.g., C<MyMessage>).

=head2 field_count()

Returns the number of fields defined in this message.

=head2 get_field($index)

Returns the L<Protobuf::Descriptor::Field> at the given C<$index> (0-based).

=head2 find_field_by_name($name)

Returns the L<Protobuf::Descriptor::Field> with the given C<$name>, or C<undef> if not found.

=head2 find_field_by_number($number)

Returns the L<Protobuf::Descriptor::Field> with the given tag C<$number>, or C<undef> if not found.

=head2 oneof_count()

Returns the number of C<oneof> declarations in this message.

=head2 get_oneof($index)

Returns the L<Protobuf::Descriptor::OneofDef> at the given C<$index> (0-based).

=head2 find_oneof_by_name($name)

Returns the L<Protobuf::Descriptor::OneofDef> with the given C<$name>, or C<undef> if not found.

=head2 nested_message_count()

Returns the number of nested message types defined within this message.

=head2 get_nested_message($index)

Returns the L<Protobuf::Descriptor::MessageDef> for the nested message at the given C<$index> (0-based).

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Descriptor>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Descriptor::MessageDef;

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

sub find_field_by_name {
    my ($self, $name) = @_;
    return _xs_find_field_by_name($self, $name);
}

sub find_field_by_number {
    my ($self, $number) = @_;
    return _xs_find_field_by_number($self, $number);
}

sub oneof_count {
    my ($self) = @_;
    return _xs_oneof_count($self);
}

sub get_oneof {
    my ($self, $index) = @_;
    return _xs_oneof($self, $index);
}

sub find_oneof_by_name {
    my ($self, $name) = @_;
    return _xs_find_oneof_by_name($self, $name);
}

sub nested_message_count {
    my ($self) = @_;
    return _xs_nested_message_count($self);
}

sub get_nested_message {
    my ($self, $index) = @_;
    return _xs_nested_message($self, $index);
}

sub nested_enum_count {
    my ($self) = @_;
    return _xs_nested_enum_count($self);
}

sub get_nested_enum {
    my ($self, $index) = @_;
    return _xs_nested_enum($self, $index);
}

sub file {
    my ($self) = @_;
    return _xs_file($self);
}

sub perl_class_name {
    my ($self) = @_;
    return _xs_perl_class_name($self);
}

__PACKAGE__->meta->make_immutable;

1;
