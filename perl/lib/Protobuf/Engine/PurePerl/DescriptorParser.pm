package Protobuf::Engine::PurePerl::DescriptorParser;

use strict;
use warnings;
use Carp qw(croak);
use Protobuf::Engine::PurePerl;

# Bootstrap parser for google/protobuf/descriptor.proto
# This allows the PurePerl engine to load its own schemas.

sub parse_file_descriptor_set {
    my ($class, $data) = @_;
    my $pos = 0;
    my $files = [];
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1 && $wire == 2) { # file
            my $file_data = _read_bytes(\$data, \$pos);
            push @$files, $class->parse_file_descriptor($file_data);
        } else {
            _skip_field(\$data, \$pos, $wire);
        }
    }
    return $files;
}

sub parse_file_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $fd = {
        name => '',
        package => '',
        message_type => [],
        enum_type => [],
    };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { # name
            $fd->{name} = _read_bytes(\$data, \$pos);
        } elsif ($tag == 2) { # package
            $fd->{package} = _read_bytes(\$data, \$pos);
        } elsif ($tag == 4) { # message_type
            my $m_data = _read_bytes(\$data, \$pos);
            push @{$fd->{message_type}}, $class->parse_descriptor($m_data);
        } elsif ($tag == 5) { # enum_type
            my $e_data = _read_bytes(\$data, \$pos);
            push @{$fd->{enum_type}}, $class->parse_enum_descriptor($e_data);
        } else {
            _skip_field(\$data, \$pos, $wire);
        }
    }
    return $fd;
}

sub parse_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $m = {
        name => '',
        field => [],
        nested_type => [],
        enum_type => [],
        oneof_decl => [],
        options => {},
    };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { $m->{name} = _read_bytes(\$data, \$pos); }
        elsif ($tag == 2) {
            my $f_data = _read_bytes(\$data, \$pos);
            push @{$m->{field}}, $class->parse_field_descriptor($f_data);
        }
        elsif ($tag == 3) {
            my $n_data = _read_bytes(\$data, \$pos);
            push @{$m->{nested_type}}, $class->parse_descriptor($n_data);
        }
        elsif ($tag == 4) {
            my $e_data = _read_bytes(\$data, \$pos);
            push @{$m->{enum_type}}, $class->parse_enum_descriptor($e_data);
        }
        elsif ($tag == 7) { # options
            my $o_data = _read_bytes(\$data, \$pos);
            $m->{options} = $class->parse_message_options($o_data);
        }
        elsif ($tag == 8) { # oneof_decl
            my $o_data = _read_bytes(\$data, \$pos);
            push @{$m->{oneof_decl}}, $class->parse_oneof_descriptor($o_data);
        }
        else { _skip_field(\$data, \$pos, $wire); }
    }
    return $m;
}

sub parse_oneof_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $o = { name => '' };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { $o->{name} = _read_bytes(\$data, \$pos); }
        else { _skip_field(\$data, \$pos, $wire); }
    }
    return $o;
}

sub parse_field_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $f = { name => '', number => 0, label => 0, type => 0, type_name => '', oneof_index => undef };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { $f->{name} = _read_bytes(\$data, \$pos); }
        elsif ($tag == 3) { $f->{number} = _read_varint(\$data, \$pos); }
        elsif ($tag == 4) { $f->{label} = _read_varint(\$data, \$pos); }
        elsif ($tag == 5) { $f->{type} = _read_varint(\$data, \$pos); }
        elsif ($tag == 6) { $f->{type_name} = _read_bytes(\$data, \$pos); }
        elsif ($tag == 9) { $f->{oneof_index} = _read_varint(\$data, \$pos); }
        else { _skip_field(\$data, \$pos, $wire); }
    }
    return $f;
}

sub parse_enum_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $e = { name => '', value => [] };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { $e->{name} = _read_bytes(\$data, \$pos); }
        elsif ($tag == 2) {
            my $v_data = _read_bytes(\$data, \$pos);
            push @{$e->{value}}, $class->parse_enum_value_descriptor($v_data);
        }
        else { _skip_field(\$data, \$pos, $wire); }
    }
    return $e;
}

sub parse_enum_value_descriptor {
    my ($class, $data) = @_;
    my $pos = 0;
    my $v = { name => '', number => 0 };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 1) { $v->{name} = _read_bytes(\$data, \$pos); }
        elsif ($tag == 2) { $v->{number} = _read_varint(\$data, \$pos); }
        else { _skip_field(\$data, \$pos, $wire); }
    }
    return $v;
}

sub parse_message_options {
    my ($class, $data) = @_;
    my $pos = 0;
    my $opts = { map_entry => 0 };
    while ($pos < length($data)) {
        my ($tag, $wire) = _read_tag(\$data, \$pos);
        if ($tag == 7) { # map_entry
            $opts->{map_entry} = _read_varint(\$data, \$pos) ? 1 : 0;
        } else {
            _skip_field(\$data, \$pos, $wire);
        }
    }
    return $opts;
}

sub _read_tag {
    my ($data_ref, $pos_ref) = @_;
    my $tag_wire = Protobuf::Engine::PurePerl::_decode_varint($data_ref, $pos_ref);
    return ($tag_wire >> 3, $tag_wire & 0x07);
}

sub _read_varint {
    my ($data_ref, $pos_ref) = @_;
    return Protobuf::Engine::PurePerl::_decode_varint($data_ref, $pos_ref);
}

sub _read_bytes {
    my ($data_ref, $pos_ref) = @_;
    my $len = _read_varint($data_ref, $pos_ref);
    my $bytes = substr($$data_ref, $$pos_ref, $len);
    $$pos_ref += $len;
    return $bytes;
}

sub _skip_field {
    my ($data_ref, $pos_ref, $wire) = @_;
    if ($wire == 0) { _read_varint($data_ref, $pos_ref); }
    elsif ($wire == 1) { $$pos_ref += 8; } # 64-bit
    elsif ($wire == 2) {
        my $len = _read_varint($data_ref, $pos_ref);
        $$pos_ref += $len;
    }
    elsif ($wire == 5) { $$pos_ref += 4; } # 32-bit
    else { croak "Unsupported wire type $wire at $$pos_ref"; }
}

1;
