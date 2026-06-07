package Protobuf::Engine::XS;

use strict;
use warnings;
use parent 'Protobuf::Engine';

sub create_message {
    my ($self, $class, $mdef, $arena, $flags) = @_;
    if ($arena) {
        return Protobuf::Message::_xs_new_from_def_in_arena($mdef, $arena, $flags);
    } else {
        return Protobuf::Message::_xs_new_from_def($mdef, $flags);
    }
}

sub get {
    my ($self, $msg, $field_name) = @_;
    return $msg->{_wrappers}{$field_name} if exists $msg->{_wrappers}{$field_name};

    my $val = Protobuf::Message::_xs_get($msg, $field_name);

    if (ref($val) && ref($val) =~ /^Protobuf::Internal::(?:Repeated|Map)$/) {
        my $public_class = ref($val) . '::Public';
        my $proxy;
        if (ref($val) eq 'Protobuf::Internal::Repeated') {
            tie @$proxy, 'Protobuf::Internal::Repeated', $val;
            return $msg->{_wrappers}{$field_name} = bless $proxy, $public_class;
        }
        if (ref($val) eq 'Protobuf::Internal::Map') {
            tie %$proxy, 'Protobuf::Internal::Map', $val;
            return $msg->{_wrappers}{$field_name} = bless $proxy, $public_class;
        }
    }

    return $val;
}

sub set {
    my ($self, $msg, $field_name, $value) = @_;
    return Protobuf::Message::_xs_set($msg, $field_name, $value);
}

sub has {
    my ($self, $msg, $field_name) = @_;
    return Protobuf::Message::_xs_has($msg, $field_name);
}

sub clear {
    my ($self, $msg, $field_name) = @_;
    return Protobuf::Message::_xs_clear($msg, $field_name);
}

sub parse {
    my ($self, $class, $data, $options) = @_;
    return Protobuf::Message::_xs_parse($class, $data, $options);
}

sub serialize {
    my ($self, $msg) = @_;
    return Protobuf::Message::_xs_serialize($msg);
}

sub merge {
    my ($self, $dst, $src) = @_;
    return Protobuf::Message::_xs_merge_from($dst, $src);
}

sub copy {
    my ($self, $dst, $src) = @_;
    return Protobuf::Message::_xs_copy_from($dst, $src);
}

sub to_json {
    my ($self, $msg) = @_;
    return Protobuf::Message::_xs_to_json($msg);
}

sub from_json {
    my ($self, $class, $json, $options) = @_;
    return Protobuf::Message::_xs_from_json($class, $json, $options);
}

sub to_perl {
    my ($self, $msg) = @_;
    return Protobuf::Message::_xs_to_perl($msg);
}

sub to_text {
    my ($self, $msg) = @_;
    return Protobuf::Message::_xs_to_text($msg);
}

sub THAW {
    my ($self) = @_;
    return $self;
}

1;
