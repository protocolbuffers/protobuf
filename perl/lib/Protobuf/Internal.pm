=encoding UTF-8

=head1 NAME

Protobuf::Internal - Internal XS functions and utilities

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    use Protobuf::Internal;

    # Typically, the functions here are called by other Protobuf modules
    # and not directly by end users.

=head1 DESCRIPTION

This module serves as the primary interface to the C/XS layer for the L<Protobuf> distribution. It uses L<XSLoader> to load functions implemented in C, which interact directly with the C<upb> library.

Key functionalities provided through this module include:

=over 4

=item * Initializing the per-interpreter registry (C<init_registry>).

=item * Accessing and managing the object cache (C<get_obj_cache>, C<set_cache_capacity>).

=item * Retrieving cache audit logs and contention statistics (C<get_cache_audit_log>, C<get_contention_stats>).

=item * Core type conversion functions between Perl SVs and C<upb> types.

=back

=head1 METHODS

Most functions in this module are not intended for public use. The following are called during initialization:

=head2 init_registry()

Called once when L<Protobuf> is loaded to initialize the per-interpreter C-level registry, which holds the object cache, audit log, and other global states for the current Perl interpreter.

=head1 INTERNAL XS FUNCTIONS

The following functions are exposed from the XS layer but are for internal use by other C<Protobuf::*> modules only:

=over 4

=item * C<class_name_to_full_name>

=item * C<clear_cache>

=item * C<delete_cache_entry>

=item * C<delete_cache_ptr>

=item * C<find_by_fingerprint>

=item * C<full_name_to_class_name>

=item * C<get_cache_audit_log>

=item * C<get_cache_capacity>

=item * C<get_contention_stats>

=item * C<get_cpu_features>

=item * C<get_fingerprint>

=item * C<preallocate_arena>

=item * C<register_fingerprint>

=item * C<set_cache_capacity>

=item * C<set_chaos_enabled>

=item * C<set_chaos_params>

=item * C<class_name_to_full_name>

Converts a Perl class name (e.g., C<My::Package::Message>) to a fully qualified protobuf message name (e.g., C<my.package.Message>).

=item * C<clear_cache>

Clears the internal object cache.

=item * C<delete_cache_entry>

Deletes a specific entry from the object cache.

=item * C<delete_cache_ptr>

Deletes a cache entry based on a pointer.

=item * C<find_by_fingerprint>

Finds an object in the cache by its fingerprint.

=item * C<full_name_to_class_name>

Converts a fully qualified protobuf message name to a Perl class name.

=item * C<get_cache_audit_log>

Retrieves the cache audit log.

=item * C<get_cache_capacity>

Gets the current capacity of the object cache.

=item * C<get_contention_stats>

Retrieves cache contention statistics.

=item * C<get_cpu_features>

Detects CPU features relevant to upb.

=item * C<get_fingerprint>

Gets the fingerprint for a given object.

=item * C<preallocate_arena>

Preallocates memory within an arena.

=item * C<register_fingerprint>

Registers an object's fingerprint in the cache.

=item * C<set_cache_capacity>

Sets the capacity of the object cache.

=item * C<set_chaos_enabled>

Enables or disables chaos testing features.

=item * C<set_chaos_params>

Sets parameters for chaos testing.

=item * C<verify_binary_diff>

Verifies binary differences between messages.

=back

=head1 SEE ALSO

L<Protobuf>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::Internal;

use strict;
require Protobuf;
use warnings;
use Exporter qw(import);

our $VERSION = '0.05';

our @EXPORT_OK = qw(
    class_name_to_full_name
    clear_cache
    delete_cache_entry
    delete_cache_ptr
    find_by_fingerprint
    full_name_to_class_name
    get_cache_audit_log
    get_cache_capacity
    get_contention_stats
    get_cpu_features
    get_fingerprint
    preallocate_arena
    register_fingerprint
    set_cache_capacity
    set_chaos_enabled
    set_chaos_params
    verify_binary_diff
);

our %EXPORT_TAGS = (
    all => \@EXPORT_OK,
);


# wrap_repeated and wrap_map are used by XS to provide Public wrappers
sub wrap_repeated {
    my ($val) = @_;
    return $val unless ref($val) eq 'Protobuf::Internal::Repeated';
    return $val->{_public} if $val->{_public};
    my $proxy;
    tie @$proxy, 'Protobuf::Internal::Repeated', $val;
    return $val->{_public} = bless \@$proxy, 'Protobuf::Internal::Repeated::Public';
}

sub wrap_map {
    my ($val) = @_;
    return $val unless ref($val) eq 'Protobuf::Internal::Map';
    return $val->{_public} if $val->{_public};
    my $proxy;
    tie %$proxy, 'Protobuf::Internal::Map', $val;
    return $val->{_public} = bless \%$proxy, 'Protobuf::Internal::Map::Public';
}

1;
