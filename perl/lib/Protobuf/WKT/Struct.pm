=encoding UTF-8

=head1 NAME

Protobuf::WKT::Struct - Mixin for google.protobuf.Struct, Value, and ListValue

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # In your .proto
    import "google/protobuf/struct.proto";

    message MyConfig {
      google.protobuf.Struct params = 1;
    }

    # In your Perl code
    my $config = MyConfig->new;
    my $struct = $config->params;

    # Using from_perl to populate
    $struct->from_perl({
        foo => "bar",
        baz => [1, 2.5, true],
        nested => { key => "val" }
    });

    # Using to_perl to convert back
    my $hash_ref = $struct->to_perl();
    # $hash_ref is now { foo => "bar", baz => [1, 2.5, 1], nested => { key => "val" } }

    # Converting to JSON
    my $json_string = $struct->to_json;

=head1 DESCRIPTION

This module provides helper methods for the generated classes corresponding to the C<google.protobuf.Struct>, C<google.protobuf.Value>, and C<google.protobuf.ListValue> Well-Known Types. These methods are injected into the respective classes (C<Protobuf::WKT::Struct>, C<Protobuf::WKT::Value>, C<Protobuf::WKT::ListValue>) which are used when C<google/protobuf/struct.proto> is processed.

These types allow representing arbitrary JSON-like structures within protobuf messages.

=head1 METHODS for Protobuf::WKT::Struct

=head2 to_perl()

Converts the C<Struct> message into a Perl HASH reference. Keys are strings, and values are recursively converted from C<Protobuf::WKT::Value> objects to their Perl equivalents (scalars, ARRAY refs, HASH refs).

=head2 from_perl($hash_ref)

Populates the C<Struct> message from a Perl HASH reference. The keys of the hash become the keys in the C<fields> map. The values of the hash are recursively converted into C<Protobuf::WKT::Value> messages.

Dies if C<$hash_ref> is not a HASH reference.

Returns C<$self> for chaining.

=head2 to_json()

Serializes the C<Struct> message to a JSON string. This is equivalent to encoding the result of C<to_perl()> using L<JSON::MaybeXS>.

=head1 METHODS for Protobuf::WKT::Value

=head2 to_perl()

Converts the C<Value> message to its native Perl representation based on which field in the C<kind> oneof is set:

=over 4

=item * null_value: C<undef>

=item * number_value: Perl number

=item * string_value: Perl string

=item * bool_value: Perl boolean (1 or 0)

=item * struct_value: HASH reference (recursively converted)

=item * list_value: ARRAY reference (recursively converted)

=back

=head2 from_perl($scalar)

Populates the C<Value> message from a Perl scalar, ARRAY ref, or HASH ref.

=over 4

=item * C<undef>: sets C<null_value>

=item * HASH ref: sets C<struct_value> (recursively converted)

=item * ARRAY ref: sets C<list_value> (recursively converted)

=item * Boolean (from L<JSON::MaybeXS::is_bool>): sets C<bool_value>

=item * Looks like a number: sets C<number_value>

=item * Otherwise: sets C<string_value>

=back

Returns C<$self> for chaining.

=head1 METHODS for Protobuf::WKT::ListValue

=head2 to_perl()

Converts the C<ListValue> message into a Perl ARRAY reference. Each element is recursively converted using C<Protobuf::WKT::Value->to_perl()D>.

=head2 from_perl($array_ref)

Populates the C<ListValue> message from a Perl ARRAY reference. Each element in the array is converted into a C<Protobuf::WKT::Value> message using C<from_perl()D>.

Dies if C<$array_ref> is not an ARRAY reference.

Returns C<$self> for chaining.

=head2 get_injected_methods()

Internal method used by L<Protobuf::ClassGenerator> to list methods to inject into the class.

=head2 memory_profile()

Internal method for memory profiling.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::Message>, L<JSON::MaybeXS>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::WKT::Struct;

use strict;
use warnings;
use JSON::MaybeXS;
use Scalar::Util qw(reftype);

sub to_perl {
    my ($self) = @_;
    my $fields = $self->fields;
    my %hash;
    foreach my $key (keys %$fields) {
        $hash{$key} = $fields->{$key}->to_perl();
    }
    return \%hash;
}

sub from_perl {
    my ($self, $data) = @_;
    if (!ref($self)) { $self = $self->new(); }
    my $rt = reftype($data) || '';
    die 'Struct::from_perl expects a HASH ref' unless $rt eq 'HASH';
    my $fields = $self->fields;
    %$fields = ();
    foreach my $key (keys %$data) {
        my $val = 'Google::Protobuf::Struct::Value'->new();
        $val->from_perl($data->{$key});
        $fields->{$key} = $val;
    }
    return $self;
}

sub memory_profile {
    return { arena_bytes => 0, field_count => 0 };
}

sub to_json {
    my ($self) = @_;
    return JSON::MaybeXS->new(utf8 => 1)->encode($self->to_perl);
}

sub get_injected_methods {
    return qw(to_perl from_perl memory_profile to_json);
}

{
    package Protobuf::WKT::Value; ## no critic (Modules::ProhibitMultiplePackages)
    use strict;
    use warnings;
    use Scalar::Util qw(reftype);

    sub to_perl {
        my ($self) = @_;
        my $kind = $self->which_oneof('kind');
        return unless defined $kind;
        if ($kind eq 'null_value') { return; }
        if ($kind eq 'number_value') { return $self->number_value; }
        if ($kind eq 'string_value') { return $self->string_value; }
        if ($kind eq 'bool_value') { return $self->bool_value; }
        if ($kind eq 'struct_value') { return $self->struct_value->to_perl(); }
        if ($kind eq 'list_value') { return $self->list_value->to_perl(); }
        return;
    }

    sub from_perl {
        my ($self, $val) = @_;
        if (!ref($self)) { $self = $self->new(); }
        my $rt = reftype($val) || '';
        if (!defined $val) { $self->null_value(0); }
        elsif ($rt eq 'HASH') {
            my $s = 'Google::Protobuf::Struct::Struct'->new();
            $s->from_perl($val);
            $self->struct_value($s);
        }
        elsif ($rt eq 'ARRAY') {
            my $l = 'Google::Protobuf::Struct::ListValue'->new();
            $l->from_perl($val);
            $self->list_value($l);
        }
        elsif ($rt eq '') {
            if (JSON::MaybeXS::is_bool($val)) {
                $self->bool_value($val ? 1 : 0);
            }
            elsif ($val =~ /^-?\d+(\.\d+)?$/) { $self->number_value($val); }
            else { $self->string_value($val); }
        }
        return $self;
    }

    sub get_injected_methods {
        return qw(to_perl from_perl);
    }
}

{
    package Protobuf::WKT::ListValue; ## no critic (Modules::ProhibitMultiplePackages)
    use strict;
    use warnings;
    use Scalar::Util qw(reftype);

    sub to_perl {
        my ($self) = @_;
        my $values = $self->values;
        my @list;
        foreach my $v (@$values) {
            push @list, $v->to_perl();
        }
        return \@list;
    }

    sub from_perl {
        my ($self, $data) = @_;
        if (!ref($self)) { $self = $self->new(); }
        my $rt = reftype($data) || '';
        die 'ListValue::from_perl expects an ARRAY ref' unless $rt eq 'ARRAY';
        my $values = $self->values;
        @$values = ();
        foreach my $v (@$data) {
            my $val = 'Google::Protobuf::Struct::Value'->new();
            $val->from_perl($v);
            push @$values, $val;
        }
        return $self;
    }

    sub get_injected_methods {
        return qw(to_perl from_perl);
    }
}

1;
