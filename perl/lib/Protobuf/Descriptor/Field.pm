=encoding UTF-8

=head1 NAME

Protobuf::Descriptor::Field - Descriptor for a field in a Protocol Buffer message

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $msg_def = $pool->find_message_by_name('my.package.MyMessage');
    my $field_def = $msg_def->find_field_by_name('my_field');

    if ($field_def) {
        print "Field Name: ", $field_def->name, "
";
        print "Type: ", $field_def->type, "
";
        print "Label: ", $field_def->label, "
";
        if ($field_def->message_type) {
            print "Message Type: ", $field_def->message_type->full_name, "
";
        }
    }

=head1 DESCRIPTION

This class represents the descriptor for a single field within a Protocol Buffer message. It provides methods to introspect the field's properties, such as its name, type, label (required, optional, repeated), and number.

Instances of this class are usually obtained from a L<Protobuf::Descriptor::MessageDef>.

=head1 METHODS

=head2 name()

Returns the name of the field.

=head2 full_name()

Returns the fully qualified name of the field.

=head2 number()

Returns the tag number of the field.

=head2 type()

Returns a string representing the field's type (e.g., 'double', 'int32', 'string', 'message', 'enum').

=head2 label()

Returns a string indicating the field's label: 'optional', 'required', or 'repeated'.

=head2 is_repeated()

Returns true if the field is a repeated field.

=head2 is_map()

Returns true if the field is a map field.

=head2 is_required()

Returns true if the field is a required field.

=head2 message_type()

If the field's type is 'message', returns the L<Protobuf::Descriptor::MessageDef> for that message type. Otherwise, returns C<undef>.

=head2 enum_type()

If the field's type is 'enum', returns the L<Protobuf::Descriptor::Enum> for that enum type. Otherwise, returns C<undef>.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Descriptor>, L<Protobuf::Descriptor::MessageDef>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Descriptor::Field;

use Moo;
use strict;
require Protobuf;
use warnings;

our $VERSION = '0.05';


sub name {
    my ($self) = @_;
    return _xs_name($self);
}

sub full_name {
    my ($self) = @_;
    return _xs_full_name($self);
}

sub number {
    my ($self) = @_;
    return _xs_number($self);
}

sub type {
    my ($self) = @_;
    return _xs_type_name($self);
}

sub type_number {
    my ($self) = @_;
    return _xs_type($self);
}

sub label {
    my ($self) = @_;
    return _xs_label_name($self);
}

sub label_number {
    my ($self) = @_;
    return _xs_label($self);
}

sub is_repeated {
    my ($self) = @_;
    return _xs_is_repeated($self);
}

sub is_map {
    my ($self) = @_;
    return _xs_is_map($self);
}

sub is_required {
    my ($self) = @_;
    return _xs_is_required($self);
}

sub is_extension {
    my ($self) = @_;
    return _xs_is_extension($self);
}

sub is_packed {
    my ($self) = @_;
    return _xs_is_packed($self);
}

sub message_type {
    my ($self) = @_;
    return _xs_message_type($self);
}

sub enum_type {
    my ($self) = @_;
    return _xs_enum_type($self);
}

__PACKAGE__->meta->make_immutable;

1;
