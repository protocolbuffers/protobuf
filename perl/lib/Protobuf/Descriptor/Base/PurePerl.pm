package Protobuf::Descriptor::Base::PurePerl;

use strict;
use warnings;

our $VERSION = '0.05';

sub new {
    my ($class, $data) = @_;
    my $self = {
        _data => $data || {},
    };
    return bless $self, $class;
}

sub full_name {
    my ($self) = @_;
    return $self->{_data}{full_name};
}

1;
