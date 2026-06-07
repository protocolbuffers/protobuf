package Protobuf::Engine;

use strict;
use warnings;
use Carp qw(croak);

sub new {
    my ($class) = @_;
    return bless {}, $class;
}

# Abstract Methods

sub create_message { croak "Abstract method" }
sub parse          { croak "Abstract method" }
sub serialize      { croak "Abstract method" }
sub merge          { croak "Abstract method" }
sub copy           { croak "Abstract method" }
sub to_json        { croak "Abstract method" }
sub from_json      { croak "Abstract method" }
sub to_perl        { croak "Abstract method" }
sub to_text        { croak "Abstract method" }

1;
