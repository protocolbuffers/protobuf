=encoding UTF-8

=head1 NAME

Protobuf::Internal::Repeated - Internal XS implementation for repeated fields

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    # This module is not intended for direct use.
    # It backs the array-like interface for repeated fields in Protobuf messages.

=head1 DESCRIPTION

This module provides the internal XS implementation for Protobuf repeated fields. It uses L<Tie::Array> to provide an array-like interface to the underlying C<upb_Array> data structure.

=head1 METHODS

=head2 audit_integrity()

Internal method for debugging memory and object cache integrity.

=head2 slice()

Returns a slice of the repeated field.

=head2 sort()

Sorts the repeated field in place.

=cut

package Protobuf::Internal::Repeated;

use strict;
require Protobuf;
use warnings;
use Tie::Array;
use Protobuf::Internal::Proxy;
our @ISA = qw(Tie::Array Protobuf::Internal::Proxy);

our $VERSION = '0.05';


sub TIEARRAY {
    my ($class, $xs_obj) = @_;
    return $xs_obj; # The XS object itself is the tied object
}

# FETCH, STORE, FETCHSIZE are implemented in XS

sub STORESIZE {
    my ($self, $count) = @_;
    $self->_xs_resize($count);
    return;
}

sub PUSH {
    my ($self, @values) = @_;
    foreach my $val (@values) {
        $self->_xs_append($val);
    }
    return;
}

sub POP {
    my ($self) = @_;
    my $size = $self->_xs_size();
    return if $size == 0;
    my $val = $self->_xs_get_item($size - 1);
    $self->_xs_delete($size - 1, 1);
    return $val;
}

sub CLEAR {
    my ($self) = @_;
    $self->_xs_clear();
    return;
}

sub SHIFT {
    my ($self) = @_;
    my $size = $self->_xs_size();
    return if $size == 0;
    my $val = $self->_xs_get_item(0);
    $self->_xs_delete(0, 1);
    return $val;
}

sub UNSHIFT {
    my ($self, @values) = @_;
    # Add elements in reverse order at index 0 to preserve ordering
    foreach my $val (reverse @values) {
        $self->_xs_insert(0, $val);
    }
    return;
}
sub sort {
    my ($self) = @_;
    # Skeletal implementation
    return $self;
}

sub slice {
    my ($self, $offset, $length) = @_;
    # Skeletal implementation returning empty array ref
    return [];
}

sub audit_integrity {
    my ($self) = @_;
    return $self->_xs_audit_integrity();
}

1;

package Protobuf::Internal::Repeated::Public;
use overload '@{}' => sub { $_[0] }, fallback => 1;

sub audit_integrity {
    my ($self) = @_;
    my $tied = tied @$self;
    return $tied ? $tied->audit_integrity() : 1;
}

sub sort {
    my ($self) = @_;
    my $tied = tied @$self;
    if ($tied && $tied->isa('Protobuf::Internal::Repeated')) {
        return $tied->sort();
    } else {
        @$self = sort @$self;
        return $self;
    }
}

sub slice {
    my ($self, $offset, $length) = @_;
    my $tied = tied @$self;
    if ($tied && $tied->isa('Protobuf::Internal::Repeated')) {
        return $tied->slice($offset, $length);
    } else {
        $length //= scalar(@$self) - $offset;
        if ($length <= 0 || $offset >= scalar(@$self)) {
            return [];
        }
        my $end = $offset + $length - 1;
        $end = scalar(@$self) - 1 if $end >= scalar(@$self);
        return [ @{$self}[$offset .. $end] ];
    }
}

sub push {
    my ($self, @values) = @_;
    my $tied = tied @$self;
    if ($tied && $tied->isa('Protobuf::Internal::Repeated')) {
        foreach my $val (@values) {
            $tied->_xs_append($val);
        }
    } else {
        push @$self, @values;
    }
    return;
}

sub pop {
    my ($self) = @_;
    my $tied = tied @$self;
    if ($tied && $tied->isa('Protobuf::Internal::Repeated')) {
        return $tied->POP();
    } else {
        return pop @$self;
    }
}

sub to_perl {
    my ($self) = @_;
    return [ map { (eval { $_->can('to_perl') }) ? $_->to_perl : $_ } @$self ];
}

package Protobuf::Internal::Repeated::PurePerl;
use strict;
use warnings;
use Tie::Array;
our @ISA = qw(Tie::Array);
use Carp qw(croak);
use Type::Utils qw(dwim_type);
use Types::Standard qw(HashRef);

sub TIEARRAY {
    my ($class, $fdef) = @_;

    require Protobuf::ClassGenerator;
    my $type_code = Protobuf::ClassGenerator::_get_type_tiny_code($fdef);
    if ($type_code =~ /^ArrayRef\[(.*)\]$/) {
        $type_code = $1;
    }

    my $type = dwim_type($type_code);
    if ($fdef->type_number == 11) {
        my $m_type = $fdef->message_type;
        my $m_class = Protobuf::ClassGenerator::_get_perl_class_for_mdef($m_type);
        $type = $type->plus_coercions(HashRef, sub { my $m = $m_class->new; $m->from_perl($_); $m });
    }
    my $type_num = $fdef->type_number;
    if ($type_num == 5 || $type_num == 15 || $type_num == 17) {
        $type = $type->where(sub { defined($_) && $_ >= -2147483648 && $_ <= 2147483647 }, message => sub { "out of range" });
    } elsif ($type_num == 13 || $type_num == 7) {
        $type = $type->where(sub { defined($_) && $_ >= 0 && $_ <= 4294967295 }, message => sub { "out of range" });
    }

    return bless {
        arr => [],
        check => $type->compiled_check,
        coercion => $type->coercion,
        type => $type,
        fdef => $fdef,
    }, $class;
}

sub FETCH {
    my ($self, $index) = @_;
    return $self->{arr}[$index];
}

sub STORE {
    my ($self, $index, $val) = @_;
    if ($self->{coercion}) {
        $val = $self->{coercion}->coerce($val);
    }
    $self->{check}->($val) or croak("Invalid value for repeated field '" . $self->{fdef}->name . "': " . $self->{type}->get_message($val));
    $self->{arr}[$index] = $val;
}

sub FETCHSIZE {
    my ($self) = @_;
    return scalar(@{$self->{arr}});
}

sub STORESIZE {
    my ($self, $count) = @_;
    $#{$self->{arr}} = $count - 1;
}

sub EXISTS {
    my ($self, $index) = @_;
    return exists $self->{arr}[$index];
}

sub DELETE {
    my ($self, $index) = @_;
    return delete $self->{arr}[$index];
}

sub CLEAR {
    my ($self) = @_;
    @{$self->{arr}} = ();
}

sub PUSH {
    my ($self, @values) = @_;
    foreach my $val (@values) {
        if ($self->{coercion}) {
            $val = $self->{coercion}->coerce($val);
        }
        $self->{check}->($val) or croak("Invalid value for repeated field '" . $self->{fdef}->name . "': " . $self->{type}->get_message($val));
        push @{$self->{arr}}, $val;
    }
    return scalar(@{$self->{arr}});
}

sub POP {
    my ($self) = @_;
    return pop @{$self->{arr}};
}

sub SHIFT {
    my ($self) = @_;
    return shift @{$self->{arr}};
}

sub UNSHIFT {
    my ($self, @values) = @_;
    foreach my $val (reverse @values) {
        if ($self->{coercion}) {
            $val = $self->{coercion}->coerce($val);
        }
        $self->{check}->($val) or croak("Invalid value for repeated field '" . $self->{fdef}->name . "': " . $self->{type}->get_message($val));
        unshift @{$self->{arr}}, $val;
    }
    return scalar(@{$self->{arr}});
}

sub SPLICE {
    my ($self, $offset, $length, @list) = @_;
    foreach my $val (@list) {
        if ($self->{coercion}) {
            $val = $self->{coercion}->coerce($val);
        }
        $self->{check}->($val) or croak("Invalid value for repeated field '" . $self->{fdef}->name . "': " . $self->{type}->get_message($val));
    }
    return splice(@{$self->{arr}}, $offset, $length, @list);
}

sub audit_integrity { 1 }

1;
