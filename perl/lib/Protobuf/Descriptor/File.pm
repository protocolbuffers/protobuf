=encoding UTF-8

=head1 NAME

Protobuf::Descriptor::File - Descriptor for a .proto file

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $file_def = $pool->find_file_by_name('my/app/stuff.proto');

    if ($file_def) {
        print "File Name: ", $file_def->name, "
";
        print "Package: ", $file_def->get_package, "
";
        for my $i (0 .. $file_def->top_level_message_count - 1) {
            my $msg = $file_def->get_top_level_message($i);
            print "  Message: ", $msg->full_name, "
";
        }
    }

=head1 DESCRIPTION

This class represents the descriptor for a single C<.proto> file. It contains information about the file's name, package, and the top-level messages, enums, and extensions defined within it.

Instances of this class are usually obtained from a L<Protobuf::DescriptorPool> when a file is loaded, for example, via C<< $pool->add_serialized_file() >>.

=head1 METHODS

=head2 name()

Returns the path/name of the C<.proto> file.

=head2 get_package()

Returns the package name declared in the C<.proto> file.

=head2 top_level_message_count()

Returns the number of top-level message types defined in this file.

=head2 get_top_level_message($index)

Returns the L<Protobuf::Descriptor::MessageDef> for the top-level message at the given C<$index> (0-based).

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Descriptor>, L<Protobuf::Descriptor::MessageDef>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Descriptor::File;

use Moo;
use strict;
require Protobuf;
use warnings;

our $VERSION = '0.05';


# The object is a blessed IV (the pointer itself) from the C layer,
# but Moo can still work with it if we're careful.
# However, for simplicity, let's keep it as is.

sub name {
    my ($self) = @_;
    return _xs_name($self);
}

sub get_package {
    my ($self) = @_;
    return _xs_package($self);
}

sub top_level_message_count {
    my ($self) = @_;
    return _xs_top_level_message_count($self);
}

sub get_top_level_message {
    my ($self, $index) = @_;
    return _xs_top_level_message($self, $index);
}

sub top_level_enum_count {
    my ($self) = @_;
    return _xs_top_level_enum_count($self);
}

sub get_top_level_enum {
    my ($self, $index) = @_;
    return _xs_top_level_enum($self, $index);
}

__PACKAGE__->meta->make_immutable;

1;
