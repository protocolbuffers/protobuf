=encoding UTF-8

=head1 NAME

Protobuf::WKT::Any - Mixin for google.protobuf.Any message

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # In your .proto
    import "google/protobuf/any.proto";

    message MyMessage {
      google.protobuf.Any details = 1;
    }

    # In your Perl code
    my $any = $my_message->details; # Assuming this is an instance of the generated class for Any

    # Packing
    my $other_msg = Other::Message->new(foo => 'bar');
    $any->pack($other_msg);

    # Unpacking
    if ($any->type_url =~ /Other.Message$/) {
        my $unpacked = $any->unpack('Other::Message');
        # $unpacked is now an instance of Other::Message
    }

=head1 DESCRIPTION

This module provides helper methods (C<pack> and C<unpack>) for the generated class corresponding to the C<google.protobuf.Any> Well-Known Type. These methods are injected into the C<Protobuf::WKT::Any> class, which should be automatically used when C<google/protobuf/any.proto> is processed.

The C<Any> type allows embedding a message of any type within it, along with a type URL to identify the embedded message type.

=head1 METHODS

=head2 pack($msg)

Serializes the given C<$msg> (which must be a L<Protobuf::Message> instance) and stores it in the C<value> field. It also sets the C<type_url> field to C<type.googleapis.com/> followed by the full name of the message type.

Returns C<$self> for chaining.

=head2 unpack($expected_class)

Deserializes the message stored in the C<value> field. It uses the C<type_url> to determine the message type.

If C<$expected_class> is provided, it is used as the Perl class to deserialize into. Otherwise, the class is derived from the C<type_url> and looked up in the C<generated_pool>.

Returns the deserialized message instance.

Dies if the type from C<type_url> cannot be found in the descriptor pool.

=head2 get_injected_methods()

Internal method used by the class generator to know which methods to inject. Returns C<< qw(pack unpack) >>.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Message>, L<Protobuf::DescriptorPool>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::WKT::Any;

use strict;
use warnings;

sub pack { ## no critic (Subroutines::ProhibitBuiltinHomonyms)
    my ($self, $msg) = @_;

    my $mdef = $msg->descriptor();
    my $full_name = $mdef->full_name();

    # Standard type_url prefix
    my $type_url = "type.googleapis.com/$full_name";

    $self->set('type_url', $type_url);
    $self->set('value', $msg->serialize());

    return $self;
}

sub unpack { ## no critic (Subroutines::ProhibitBuiltinHomonyms)
    my ($self, $expected_class) = @_;

    my $type_url = $self->get('type_url');
    my ($full_name) = $type_url =~ m{([^/]+)$};

    # If expected_class is provided, use it. Otherwise, look up in pool.
    my $class = $expected_class;
    if (!$class) {
        require Protobuf::DescriptorPool;
        my $pool = Protobuf::DescriptorPool->generated_pool();
        my $mdef = $pool->find_message_by_name($full_name);
        if (!$mdef) {
            die "Cannot unpack Any: type $full_name not found in pool";
        }
        $class = $mdef->perl_class_name();
    }

    return $class->parse($self->get('value'));
}

sub get_injected_methods {
    return qw(pack unpack);
}

1;
