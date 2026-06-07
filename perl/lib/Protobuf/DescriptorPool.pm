=encoding UTF-8

=head1 NAME

Protobuf::DescriptorPool - Manages a collection of Protocol Buffer descriptors

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    use Protobuf::DescriptorPool;

    # Get the default pool containing descriptors from generated files
    my $pool = Protobuf::DescriptorPool->generated_pool;

    # Find a message descriptor
    my $message_def = $pool->find_message_by_name('my.package.MyMessage');
    if ($message_def) {
        print "Found: ", $message_def->full_name, "
";
    }

    # Create a new, empty pool
    my $custom_pool = Protobuf::DescriptorPool->new;

    # Add descriptors from a serialized FileDescriptorProto
    my $file_def = $custom_pool->add_serialized_file($binary_file_descriptor);

    # Add descriptors from a serialized FileDescriptorSet
    my $file_defs = $custom_pool->add_serialized_file_descriptor_set($binary_file_descriptor_set);

=head1 DESCRIPTION

C<Protobuf::DescriptorPool> is a container for Protocol Buffer descriptor objects (like message types, field definitions, enum types, etc.). It allows you to load descriptor information, typically from serialized C<FileDescriptorProto> or C<FileDescriptorSet> messages, and then look up specific descriptors by name.

This class wraps the C<upb_DefPool> functionality from the C<upb> library.

Descriptors loaded into a pool are used to understand the structure of serialized messages, enabling parsing, serialization, and dynamic message creation.

=head1 METHODS

== Class Methods

=head2 generated_pool()

    my $pool = Protobuf::DescriptorPool->generated_pool;

Returns a singleton instance of the descriptor pool that contains all the message, enum, and file descriptors that were compiled into your Perl application, typically from C<.proto> files processed by C<protoc-gen-perl-pb> at build time.

== Instance Methods

=head2 new()

    my $pool = Protobuf::DescriptorPool->new;

Creates a new, empty descriptor pool.

=head2 add_serialized_file($serialized_file_descriptor)

    my $file_def = $pool->add_serialized_file($binary_data);

Parses a binary string containing a single serialized C<google.protobuf.FileDescriptorProto> message. The descriptors defined within that file are added to the pool. On success, returns the L<Protobuf::Descriptor::File> object for the added file.

Triggers L<Protobuf::ClassGenerator> to define Perl classes for the messages in the file.

=head2 add_serialized_file_descriptor_set($serialized_file_descriptor_set)

    my $file_defs = $pool->add_serialized_file_descriptor_set($binary_data);

Parses a binary string containing a serialized C<google.protobuf.FileDescriptorSet> message. All files within the set are added to the pool. On success, returns an ArrayRef of L<Protobuf::Descriptor::File> objects.

Triggers L<Protobuf::ClassGenerator> for each file added.

=head2 find_file_by_name($name)

    my $file_def = $pool->find_file_by_name('path/to/my.proto');

Finds a L<Protobuf::Descriptor::File> by its name (usually the original C<.proto> file path). Returns C<undef> if not found.

=head2 find_message_by_name($full_name)

    my $msg_def = $pool->find_message_by_name('my.package.MyMessage');

Finds a L<Protobuf::Descriptor::MessageDef> by its fully qualified name. Returns C<undef> if not found.

=head2 find_enum_by_name($full_name)

    my $enum_def = $pool->find_enum_by_name('my.package.MyEnum');

Finds a L<Protobuf::Descriptor::Enum> by its fully qualified name. Returns C<undef> if not found.

=head2 find_extension_by_name($full_name)

    my $ext_def = $pool->find_extension_by_name('my.package.my_extension');

Finds a L<Protobuf::Descriptor::Field> representing an extension by its fully qualified name. Returns C<undef> if not found.

=head2 freeze()

(Not Yet Implemented) Marks the pool as immutable. Attempting to add more descriptors after freezing will result in an error. This is a prerequisite for potentially sharing pools across threads in the future.

=head2 is_frozen()

(Not Yet Implemented) Returns true if the pool has been frozen.

=head2 DEMOLISH()

Internal method to clean up the underlying C<upb_DefPool> and its associated arena when the Perl object is destroyed.

=head2 CLONE()

Throws an error. Descriptor pools cannot be safely cloned across threads.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Descriptor>, L<Protobuf::ClassGenerator>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::DescriptorPool;

use Moo;
use strict;
require Protobuf;
use warnings;
use Carp qw(croak);
use Log::Any qw($log);

our $VERSION = '0.05';

use Protobuf::Descriptor::File;
use Protobuf::Descriptor::MessageDef;
use Protobuf::Descriptor::Enum;
use Protobuf::Descriptor::Field;
use Protobuf::Descriptor::OneofDef;
use Protobuf::ClassGenerator;
use Protobuf::DescriptorPool::PurePerl;

has '_pp_pool' => (
    is => 'ro',
    lazy => 1,
    default => sub { Protobuf::DescriptorPool::PurePerl->new() },
);

has '_pool_ptr' => (
    is       => 'ro',
    default  => sub { $Protobuf::HAS_XS ? _xs_create_raw() : undef },
);

sub DEMOLISH {
    my $self = shift;
    if ($Protobuf::HAS_XS && exists $self->{_pool_ptr} && $self->{_pool_ptr}) {
        _xs_destroy_raw($self->{_pool_ptr});
    }
    return;
}

sub CLONE_SKIP { 1 }

my $pp_generated_pool;

sub generated_pool {
    return $Protobuf::HAS_XS ? _xs_generated_pool() : ($pp_generated_pool ||= __PACKAGE__->new());
}

sub add_serialized_file {
    my ($self, $serialized) = @_;
    croak('Serialized descriptor data is required') unless defined $serialized;

    my $file;
    if ($Protobuf::HAS_XS) {
        $file = _xs_add_serialized_file($self, $serialized);
    } else {
        $file = $self->_pp_pool->add_serialized_file($serialized);
    }

    if ($file) {
        Protobuf::ClassGenerator->generate_for_file($file);
    }
    return $file;
}

sub add_serialized_file_descriptor_set {
    my ($self, $serialized) = @_;
    croak('Serialized descriptor set data is required') unless defined $serialized;

    my $files;
    if ($Protobuf::HAS_XS) {
        $files = _xs_add_serialized_file_descriptor_set($self, $serialized);
    } else {
        $files = $self->_pp_pool->add_serialized_file_descriptor_set($serialized);
    }

    if ($files && ref($files) eq 'ARRAY') {
        $log->debug('Added ' . scalar(@$files) . ' files from descriptor set');
        foreach my $file (@$files) {
            Protobuf::ClassGenerator->generate_for_file($file);
        }
    }
    return $files;
}

sub find_file_by_name {
    my ($self, $name) = @_;
    return $Protobuf::HAS_XS ? _xs_find_file_by_name($self, $name) : $self->_pp_pool->find_file_by_name($name);
}

sub find_message_by_name {
    my ($self, $name) = @_;
    $name =~ s/::/./g;
    return $Protobuf::HAS_XS ? _xs_find_message_by_name($self, $name) : $self->_pp_pool->find_message_by_name($name);
}

sub find_enum_by_name {
    my ($self, $name) = @_;
    $name =~ s/::/./g;
    return $Protobuf::HAS_XS ? _xs_find_enum_by_name($self, $name) : $self->_pp_pool->find_enum_by_name($name);
}

sub find_service_by_name {
    my ($self, $name) = @_;
    $name =~ s/::/./g;
    return $Protobuf::HAS_XS ? _xs_find_service_by_name($self, $name) : undef; # TODO PP services
}

sub find_extension_by_name {
    my ($self, $name) = @_;
    return $Protobuf::HAS_XS ? _xs_find_extension_by_name($self, $name) : undef; # TODO PP extensions
}

sub freeze {
    my ($self) = @_;
    $self->_xs_freeze() if $Protobuf::HAS_XS;
    return 1;
}

sub is_frozen {
    my ($self) = @_;
    return $Protobuf::HAS_XS ? $self->_xs_is_frozen() : 0;
}

__PACKAGE__->meta->make_immutable;

1;
