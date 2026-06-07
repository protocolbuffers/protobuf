=encoding UTF-8

=head1 NAME

Protobuf::Arena - Memory arena for Protocol Buffer messages

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    use Protobuf::Arena;

    # Create a new arena
    my $arena = Protobuf::Arena->new;

    # Messages can be created within a specific arena
    # (This is usually handled internally when creating messages)
    # my $msg = MyMessage->new(arena => $arena);

    # tmpfs-backed arena for inter-process communication
    my $shm_arena = Protobuf::Arena->new_tmpfs('/dev/shm/my_proto_ipc', 65536);
    my $attached_arena = Protobuf::Arena->attach_tmpfs('/dev/shm/my_proto_ipc', 65536);

=head1 DESCRIPTION

This module wraps the C<upb_Arena> type from the C<upb> library. Arenas provide an efficient way to manage the memory lifecycle of Protocol Buffer messages and their submessages. All memory allocated for messages within an arena is freed at once when the arena itself is destroyed (typically when the C<Protobuf::Arena> object goes out of scope).

This approach reduces the overhead of individual allocations and deallocations, significantly improving performance, especially for complex messages or high-throughput scenarios.

Key architectural features related to arenas in this implementation:

=over 4

=item * **Ownership:** Top-level L<Protobuf::Message> objects usually create and own their C<Protobuf::Arena> instance.

=item * **Shared Memory (tmpfs):** Supports creating arenas backed by C<tmpfs> for high-performance, zero-copy Inter-Process Communication (IPC), as detailed in C<perl/doc/architecture/advanced/01-tmpfs-ipc-design.md>.

=item * **Memory Canaries:** Arenas use canary bytes (C<0xDEADBEEFCAFEBABEULL>) to detect memory corruption (buffer overflows/underflows) within arena-allocated blocks.

=item * **NUMA Awareness:** (Planned) Optimizations for memory placement on multi-socket systems.

=back

=head1 METHODS

== Class Methods

=head2 new()

    my $arena = Protobuf::Arena->new;

Creates a new, standard memory-backed arena.

=head2 new_tmpfs($path, $size)

    my $arena = Protobuf::Arena->new_tmpfs('/dev/shm/my_ipc', 1048576);

Creates a new arena backed by a file in C<tmpfs> at the given C<$path> with a specified C<$size>. This is used for shared memory IPC.

=head2 attach_tmpfs($path, $size)

    my $arena = Protobuf::Arena->attach_tmpfs('/dev/shm/my_ipc', 1048576);

Attaches to an existing C<tmpfs>-backed arena at C<$path> with the given C<$size>.

== Instance Methods

=head2 space_allocated()

Returns the total bytes currently allocated by objects within the arena.

=head2 space_reserved()

Returns the total bytes reserved by the arena from the system, including any overhead or preallocated blocks.

=head2 stats()

Returns a hash reference containing detailed memory usage statistics for the arena, such as allocated bytes, reserved bytes, and block counts.

=head2 is_tmpfs()

Returns true if the arena is backed by C<tmpfs>.

=head2 get_path()

If the arena is C<tmpfs>-backed, returns the file system path.

=head2 verify_selinux()

(Experimental) Performs checks related to SELinux contexts if being used with C<tmpfs>-backed arenas for secure IPC.

=head2 get_offset($pointer_iv)

Given an integer representing a memory address within the arena, returns the offset from the base of the arena's memory block. Used for shared memory IPC handle serialization.

=head2 attach_message($message_full_name, $offset)

(Experimental) Reifies a message object of the given type (C<$message_full_name>) from the arena memory at the specified C<$offset>. Used for shared memory IPC.

=head2 set_numa_node($node)

(Planned) Sets the preferred NUMA node for memory allocations within this arena.

=head2 DEMOLISH()

Internal method. Frees the underlying C<upb_Arena> and all memory associated with it when the Perl object is destroyed.

=head2 Clone()

Creates a new, empty C<Protobuf::Arena>. It does *not* copy the contents of the original arena.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Message>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Arena;

use Moo;
use strict;
use warnings;

use Protobuf::Internal; # Ensure audit logging is available
require Protobuf;
use Carp qw(croak);

our $VERSION = '0.05';

has '_arena_ptr' => (
    is       => 'ro',
    default  => sub { _xs_create_raw() },
);

sub new_tmpfs {
    my ($class, $path, $size) = @_;
    croak("Usage: $class->new_tmpfs(path, size)") unless defined $path && defined $size;
    return _xs_create_tmpfs_raw($path, $size);
}

sub attach_tmpfs {
    my ($class, $path, $size) = @_;
    croak("Usage: $class->attach_tmpfs(path, size)") unless defined $path && defined $size;
    return _xs_attach_tmpfs_raw($path, $size);
}

sub set_numa_node {
    my ($self, $node) = @_;
    return $self->_xs_set_numa_node($node);
}

sub CLONE_SKIP { 1 }

sub DEMOLISH {
    my $self = shift;
    $self->_xs_destroy();
    return;
}

sub Clone {
    my $self = shift;
    return Protobuf::Arena->new();
}

sub __test_canary_corruption {
    my $self = shift;
    $self->_xs_test_canary_corruption();
    return;
}

sub __test_block_canary {
    my $self = shift;
    $self->_xs_test_block_canary();
    return;
}

sub stats {
    my $self = shift;
    return $self->_xs_stats();
}

sub __chaos_fail_probability {
    my ($self, $p) = @_;
    $self->_xs_set_chaos_fail_probability($p);
}

sub __chaos_delay_probability {
    my ($self, $p) = @_;
    $self->_xs_set_chaos_delay_probability($p);
}

sub space_allocated {
    my $self = shift;
    return $self->_xs_space_allocated();
}

sub space_reserved {
    my $self = shift;
    return $self->_xs_space_reserved();
}

sub is_tmpfs {
    my $self = shift;
    return $self->_xs_is_tmpfs();
}

sub get_path {
    my $self = shift;
    return $self->_xs_get_path();
}

sub verify_selinux {
    my $self = shift;
    return $self->_xs_verify_selinux();
}

sub get_offset {
    my ($self, $ptr_iv) = @_;
    return $self->_xs_get_offset($ptr_iv);
}

sub attach_message {
    my ($self, $name, $offset) = @_;
    return $self->_xs_attach_message($name, $offset);
}

sub detach {
    my $self = shift;
    return $self->_xs_detach();
}

sub attach {
    my ($class, $ptr_iv) = @_;
    return _xs_attach($class, $ptr_iv);
}

__PACKAGE__->meta->make_immutable;

sub THAW {
    my ($self) = @_;
    return $self;
}

1;
