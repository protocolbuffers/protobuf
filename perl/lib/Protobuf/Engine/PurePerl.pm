package Protobuf::Engine::PurePerl;

use strict;
use warnings;
use parent 'Protobuf::Engine';
use Carp qw(croak);

our ($IV_MIN, $IV_MAX);
sub _init_iv_limits {
    return if defined $IV_MIN;
    require Config;
    require Math::BigInt;
    if ($Config::Config{ivsize} == 8) {
        $IV_MIN = Math::BigInt->new('-9223372036854775808');
        $IV_MAX = Math::BigInt->new('9223372036854775807');
    } else {
        $IV_MIN = Math::BigInt->new('-2147483648');
        $IV_MAX = Math::BigInt->new('2147483647');
    }
}

sub create_message {
    my ($self, $class, $mdef, $arena, $flags) = @_;

    # Internal representation for PurePerl: a HashRef
    my $data = {
        _fields => {},
        _mdef   => $mdef,
        _engine => $self,
    };

    return bless $data, $class;
}

sub get {
    my ($self, $msg, $field_name) = @_;

    unless (exists $msg->{_fields}{$field_name}) {
        my $mdef = $msg->{_mdef};
        if ($mdef) {
            my $fdef = $mdef->find_field_by_name($field_name);
            if ($fdef) {
                if ($fdef->is_map) {
                    require Protobuf::Internal::Map;
                    $msg->{_fields}{$field_name} = bless {}, 'Protobuf::Internal::Map::Public';
                } elsif ($fdef->is_repeated) {
                    require Protobuf::Internal::Repeated;
                    my $arr = [];
                    tie @$arr, 'Protobuf::Internal::Repeated::PurePerl', $fdef;
                    $msg->{_fields}{$field_name} = bless $arr, 'Protobuf::Internal::Repeated::Public';
                } else {
                    return $self->_default_value($fdef);
                }
            }
        }
    }

    return $msg->{_fields}{$field_name};
}

sub _default_value {
    my ($self, $fdef) = @_;
    my $type = $fdef->type_number;

    if ($type == 1 || $type == 2) { # DOUBLE, FLOAT
        return 0.0;
    } elsif ($type == 3 || $type == 4 || $type == 16 || $type == 18) {
        # INT64, UINT64, SFIXED64, SINT64
        return 0;
    } elsif ($type == 5 || $type == 7 || $type == 13 || $type == 15 || $type == 17) {
        # INT32, FIXED32, UINT32, SFIXED32, SINT32
        return 0;
    } elsif ($type == 8) { # BOOL
        return 0;
    } elsif ($type == 9 || $type == 12) { # STRING, BYTES
        return '';
    } elsif ($type == 14) { # ENUM
        return 0;
    } elsif ($type == 11) { # MESSAGE
        return undef;
    }
    return undef;
}

sub set {
    my ($self, $msg, $field_name, $value) = @_;

    my $mdef = $msg->{_mdef};
    if ($mdef) {
        my $fdef = $mdef->find_field_by_name($field_name);
        if ($fdef) {
            if ($fdef->type_number == 14) { # ENUM
                require Scalar::Util;
                if (defined $value && !Scalar::Util::looks_like_number($value)) {
                    my $edef = $fdef->enum_type;
                    if ($edef) {
                        my $vdef = $edef->find_value_by_name($value);
                        if ($vdef) {
                            $value = $vdef->number;
                        } else {
                            croak('Invalid enum value \'' . $value . '\' for field \'' . $field_name . '\'');
                        }
                    }
                }
            }

            # Clone repeated/map/message values to ensure independence
            if ($fdef->is_map) {
                $value = _deep_clone($value);
                if (ref($value) eq 'HASH') {
                    require Protobuf::Internal::Map;
                    bless $value, 'Protobuf::Internal::Map::Public';
                }
            } elsif ($fdef->is_repeated) {
                $value = _deep_clone($value);
                if (ref($value) eq 'ARRAY') {
                    require Protobuf::Internal::Repeated;
                    my $arr = [];
                    tie @$arr, 'Protobuf::Internal::Repeated::PurePerl', $fdef;
                    push @$arr, @$value;
                    $value = bless $arr, 'Protobuf::Internal::Repeated::Public';
                }
            } elsif ($fdef->type_number == 11) {
                $value = _deep_clone($value);
            }

            my $type_num = $fdef->type_number;
            if ($type_num == 3 || $type_num == 4 || $type_num == 16 || $type_num == 18) {
                if (defined $value && ref($value) && eval { $value->isa('Math::BigInt') }) {
                    _init_iv_limits();
                    if ($value >= $IV_MIN && $value <= $IV_MAX) {
                        $value = 0 + $value->bstr();
                    }
                }
            }
        }
    }

    $msg->{_fields}{$field_name} = $value;
}

sub has {
    my ($self, $msg, $field_name) = @_;
    return exists $msg->{_fields}{$field_name};
}

sub clear {
    my ($self, $msg, $field_name) = @_;
    delete $msg->{_fields}{$field_name};
}

# --- Varint Kernels ---

sub _encode_varint {
    my ($val) = @_;
    my $v = Math::BigInt->new($val);
    if ($v->is_negative) {
        # Protobuf negative varints are always 10 bytes (unsigned 64-bit representation)
        $v = Math::BigInt->new("18446744073709551616")->badd($v);
    }

    my $res = '';
    while ($v->bcmp(128) >= 0) {
        $res .= chr(($v->copy()->band(0x7f)->as_number) | 0x80);
        $v->brsft(7);
    }
    $res .= chr($v->as_number);
    return $res;
}

sub _decode_varint {
    my ($data_ref, $pos_ref) = @_;
    my $val = Math::BigInt->new(0);
    my $shift = 0;
    my $len = length($$data_ref);
    while (1) {
        if ($$pos_ref >= $len) {
            croak "Failed to parse message: Protobuf wire format is malformed or corrupt";
        }
        my $byte = ord(substr($$data_ref, $$pos_ref++, 1));
        my $part = Math::BigInt->new($byte & 0x7f)->blsft($shift);
        $val->bior($part);
        last unless $byte & 0x80;
        $shift += 7;
        if ($shift >= 70) {
            croak "Failed to parse message: Protobuf wire format is malformed or corrupt";
        }
    }
    return $val;
}

use Math::BigInt;

# --- ZigZag Kernels ---

sub _encode_zigzag32 {
    my ($val) = @_;
    # Use 32-bit arithmetic
    my $v = Math::BigInt->new($val);
    my $res = ($v->copy()->blsft(1))->bxor($v->copy()->brsft(31));
    return $res->as_number() & 0xFFFFFFFF;
}

sub _decode_zigzag32 {
    my ($val) = @_;
    my $v = Math::BigInt->new($val);
    my $res = ($v->copy()->brsft(1))->bxor($v->copy()->band(1)->copy()->bneg());
    my $final = $res->as_number();
    # Truncate to 32-bit signed
    return unpack('l', pack('l', $final));
}

sub _encode_zigzag64 {
    my ($val) = @_;
    my $v = Math::BigInt->new($val);
    my $res = ($v->copy()->blsft(1))->bxor($v->copy()->brsft(63));
    return $res;
}

sub _decode_zigzag64 {
    my ($val) = @_;
    my $v = ref($val) ? $val->copy : Math::BigInt->new($val);
    my $res = ($v->copy()->brsft(1))->bxor($v->copy()->band(1)->bneg());
    return $res;
}

# --- Wire Format Implementation ---

my %TYPE_TO_WIRE = (
    1  => 1, # DOUBLE -> 64-bit
    2  => 5, # FLOAT -> 32-bit
    3  => 0, # INT64 -> Varint
    4  => 0, # UINT64 -> Varint
    5  => 0, # INT32 -> Varint
    6  => 1, # FIXED64 -> 64-bit
    7  => 5, # FIXED32 -> 32-bit
    8  => 0, # BOOL -> Varint
    9  => 2, # STRING -> Length-delimited
    11 => 2, # MESSAGE -> Length-delimited
    12 => 2, # BYTES -> Length-delimited
    13 => 0, # UINT32 -> Varint
    14 => 0, # ENUM -> Varint
    15 => 5, # SFIXED32 -> 32-bit
    16 => 1, # SFIXED64 -> 64-bit
    17 => 0, # SINT32 -> Varint
    18 => 0, # SINT64 -> Varint
);

sub serialize {
    my ($self, $msg) = @_;
    my $mdef = $msg->{_mdef};
    my $fields = $msg->{_fields};
    my $res = '';

    # Get fields from mdef (handle both XS and PurePerl mdef)
    my @field_defs;
    if ($mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl')) {
        my $count = $mdef->field_count;
        for (0..$count-1) {
            my $f = $mdef->get_field($_);
            push @field_defs, {
                name => $f->name,
                number => $f->number,
                type => $f->type_number,
                label => $f->label_number,
                is_map => $f->is_map,
                fdef => $f,
            };
        }
    } elsif (ref($mdef) eq 'HASH') {
        @field_defs = map { { %$_, fdef => $_ } } @{$mdef->{field} || []};
    } else {
        my $count = $mdef->field_count;
        for (0..$count-1) {
            my $f = $mdef->get_field($_);
            push @field_defs, {
                name => $f->name,
                number => $f->number,
                type => $f->type_number,
                label => $f->label_number,
                is_map => $f->is_map,
                fdef => $f,
            };
        }
    }

    # Check required fields
    my @missing;
    foreach my $f (@field_defs) {
        if (($f->{label} || 0) == 2) { # REQUIRED
            unless (exists $fields->{$f->{name}} && defined $fields->{$f->{name}}) {
                push @missing, $f->{name};
            }
        }
    }
    if (@missing) {
        croak("Failed to serialize: Missing required fields: " . join(", ", @missing));
    }

    foreach my $f (@field_defs) {
        my $val = $fields->{$f->{name}};
        next unless defined $val;

        my $tag = $f->{number};
        my $type = $f->{type};
        my $wire = $TYPE_TO_WIRE{$type};

        if ($f->{is_map}) {
            my $entry_mdef = $f->{fdef}->message_type;
            my $key_fdef = $entry_mdef->find_field_by_number(1);
            my $val_fdef = $entry_mdef->find_field_by_number(2);

            my $key_type = $key_fdef->type_number;
            my $val_type = $val_fdef->type_number;

            my $key_wire = $TYPE_TO_WIRE{$key_type};
            my $val_wire = $TYPE_TO_WIRE{$val_type};

            foreach my $k (keys %$val) {
                my $v = $val->{$k};

                my $entry_data = '';
                $entry_data .= $self->_encode_field(1, $key_type, $key_wire, $k);
                $entry_data .= $self->_encode_field(2, $val_type, $val_wire, $v);

                $res .= _encode_varint(($tag << 3) | 2);
                $res .= _encode_varint(length($entry_data));
                $res .= $entry_data;
            }
        } elsif ($f->{label} == 3) { # Repeated
            # TODO: handle packed repeated
            foreach my $item (@$val) {
                $res .= $self->_encode_field($tag, $type, $wire, $item);
            }
        } else {
            $res .= $self->_encode_field($tag, $type, $wire, $val);
        }
    }

    if (exists $msg->{_unknown_fields} && defined $msg->{_unknown_fields}) {
        $res .= $msg->{_unknown_fields};
    }

    return $res;
}

sub _encode_field {
    my ($self, $tag, $type, $wire, $val) = @_;
    my $res = _encode_varint(($tag << 3) | $wire);

    if ($wire == 0) { # Varint
        if ($type == 17) { $val = _encode_zigzag32($val); }
        elsif ($type == 18) { $val = _encode_zigzag64($val); }
        $res .= _encode_varint($val);
    } elsif ($wire == 1) { # 64-bit
        if ($type == 1) { $res .= pack('d', $val); } # double
        else {
            # fixed64, sfixed64
            my $v = Math::BigInt->new($val);
            if ($v->is_negative) {
                $v->badd("18446744073709551616");
            }
            my $hex = $v->as_hex();
            $hex =~ s/^0x//;
            $hex = '0' x (16 - length($hex)) . $hex;
            $res .= reverse(pack('H*', $hex));
        }
    } elsif ($wire == 2) { # Length-delimited
        if ($type == 11) { # Message
            my $inner = $val->serialize();
            $res .= _encode_varint(length($inner)) . $inner;
        } else { # String, Bytes
            $res .= _encode_varint(length($val)) . $val;
        }
    } elsif ($wire == 5) { # 32-bit
        if ($type == 2) { $res .= pack('f', $val); } # float
        else {
            # fixed32, sfixed32
            my $v = Math::BigInt->new($val);
            if ($v->is_negative) {
                $v->badd("4294967296");
            }
            $res .= pack('V', $v->as_number & 0xFFFFFFFF);
        }
    }
    return $res;
}

sub parse {
    my ($self, $class, $data, $options) = @_;
    my $mdef = $class->descriptor;
    my $msg = $self->create_message($class, $mdef);
    $self->parse_into($msg, $data);
    return $msg;
}

sub parse_into {
    my ($self, $msg, $data) = @_;
    my $mdef = $msg->{_mdef};
    my $fields = $msg->{_fields};

    my $pos = 0;
    my $len = length($data);

    # Pre-index fields by number for fast lookup
    my %fields_by_num;
    if ($mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl')) {
        my $count = $mdef->field_count;
        for (0..$count-1) {
            my $f = $mdef->get_field($_);
            $fields_by_num{$f->number} = {
                name => $f->name,
                type => $f->type_number,
                label => $f->label_number,
                is_map => $f->is_map,
                fdef => $f,
                message_type => ($f->type_number == 11) ? $f->message_type : undef,
            };
        }
    } elsif (ref($mdef) eq 'HASH') {
        %fields_by_num = map { $_->{number} => { %$_, fdef => $_ } } @{$mdef->{field} || []};
    } else {
        my $count = $mdef->field_count;
        for (0..$count-1) {
            my $f = $mdef->get_field($_);
            $fields_by_num{$f->number} = {
                name => $f->name,
                type => $f->type_number,
                label => $f->label_number,
                is_map => $f->is_map,
                fdef => $f,
                message_type => ($f->type_number == 11) ? $f->message_type : undef,
            };
        }
    }

    while ($pos < $len) {
        my $field_start = $pos;
        my $tag_wire = _decode_varint(\$data, \$pos);
        $tag_wire = $tag_wire->as_number();
        my $tag = $tag_wire >> 3;
        my $wire = $tag_wire & 0x07;

        my $f = $fields_by_num{$tag};
        if (!$f) {
            _skip_field(\$data, \$pos, $wire);
            my $field_end = $pos;
            my $raw_field = substr($data, $field_start, $field_end - $field_start);
            $msg->{_unknown_fields} ||= '';
            $msg->{_unknown_fields} .= $raw_field;
            next;
        }

        my $val = $self->_decode_field(\$data, \$pos, $wire, $f);
        if ($f->{is_map}) {
            my $key = $val->get('key');
            my $v = $val->get('value');
            $fields->{$f->{name}} ||= bless {}, 'Protobuf::Internal::Map::Public';
            $fields->{$f->{name}}{$key} = $v;
        } elsif ($f->{label} == 3) {
            $fields->{$f->{name}} ||= bless [], 'Protobuf::Internal::Repeated::Public';
            push @{$fields->{$f->{name}}}, $val;
        } else {
            $fields->{$f->{name}} = $val;
        }
    }
    return;
}

sub _decode_field {
    my ($self, $data_ref, $pos_ref, $wire, $f) = @_;
    my $type = $f->{type};

    if ($wire == 0) { # Varint
        my $val = _decode_varint($data_ref, $pos_ref);
        if ($type == 17) { $val = _decode_zigzag32($val); }
        elsif ($type == 18) { $val = _decode_zigzag64($val); }
        elsif ($type == 5 || $type == 13 || $type == 8 || $type == 14) {
            # 32-bit or bool or enum - convert to native if possible
            if ($type == 5) {
                # int32: if it's > 2^31-1, it's negative (it was encoded as 10-byte varint)
                if ($val->bcmp("2147483647") > 0) {
                     $val = $val->copy->bsub("18446744073709551616");
                }
                $val = $val->as_number();
            } else {
                $val = $val->as_number();
            }
        } elsif ($type == 3 || $type == 4) {
             # int64/uint64
             if ($type == 3 && $val->bcmp("9223372036854775807") > 0) {
                 $val = $val->copy->bsub("18446744073709551616");
             }
             # Keep as BigInt if it's very large, otherwise as_number
             if ($val->bcmp("9007199254740992") > 0 || $val->bcmp("-9007199254740992") < 0) {
                 # Stay as BigInt
             } else {
                 $val = $val->as_number();
             }
        }
        return $val;

    } elsif ($wire == 1) { # 64-bit
        if ($$pos_ref + 8 > length($$data_ref)) {
            croak "Failed to parse message: Protobuf wire format is malformed or corrupt";
        }
        my $bytes = substr($$data_ref, $$pos_ref, 8);
        $$pos_ref += 8;
        if ($type == 1) { return unpack('d', $bytes); }

        my $val = Math::BigInt->from_hex('0x' . unpack('H*', reverse($bytes)));
        if ($type == 16) { # sfixed64
            if ($val->bcmp("9223372036854775807") > 0) {
                $val->bsub("18446744073709551616");
            }
        }
        return ($val->bcmp("9007199254740992") > 0 || $val->bcmp("-9007199254740992") < 0) ? $val : $val->as_number();

    } elsif ($wire == 2) { # Length-delimited
        my $len = _decode_varint($data_ref, $pos_ref);
        $len = $len->as_number();
        if ($$pos_ref + $len > length($$data_ref)) {
            croak "Failed to parse message: Protobuf wire format is malformed or corrupt";
        }
        my $bytes = substr($$data_ref, $$pos_ref, $len);
        $$pos_ref += $len;
        if ($type == 11) { # Message
            my $sub_class = $self->_get_perl_class_for_mdef($f->{message_type});
            return $sub_class->parse($bytes, { profile => 'pure_perl' });
        }
        return $bytes;

    } elsif ($wire == 5) { # 32-bit
        if ($$pos_ref + 4 > length($$data_ref)) {
            croak "Failed to parse message: Protobuf wire format is malformed or corrupt";
        }
        my $bytes = substr($$data_ref, $$pos_ref, 4);
        $$pos_ref += 4;
        if ($type == 2) { return unpack('f', $bytes); }
        if ($type == 15) { return unpack('l', $bytes); } # sfixed32
        return unpack('V', $bytes);
    }

    return undef;
}

sub _get_perl_class_for_mdef {
    my ($self, $mdef) = @_;
    return undef unless $mdef;

    # XS mdef
    return $mdef->perl_class_name() if $mdef->can('perl_class_name');

    # PurePerl mdef (Class)
    if ($mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl')) {
        return $mdef->perl_class_name();
    }

    # Fallback for PurePerl mdef (HashRef)
    return $mdef->{perl_class};
}

# Helper from DescriptorParser (we should probably move these to a common Utility module)
sub _skip_field {
    my ($data_ref, $pos_ref, $wire) = @_;
    my $len = length($$data_ref);
    if ($wire == 0) { _decode_varint($data_ref, $pos_ref); }
    elsif ($wire == 1) {
        if ($$pos_ref + 8 > $len) { croak "Failed to parse message: Protobuf wire format is malformed or corrupt"; }
        $$pos_ref += 8;
    }
    elsif ($wire == 2) {
        my $l = _decode_varint($data_ref, $pos_ref);
        $l = $l->as_number();
        if ($$pos_ref + $l > $len) { croak "Failed to parse message: Protobuf wire format is malformed or corrupt"; }
        $$pos_ref += $l;
    }
    elsif ($wire == 5) {
        if ($$pos_ref + 4 > $len) { croak "Failed to parse message: Protobuf wire format is malformed or corrupt"; }
        $$pos_ref += 4;
    }
    else { croak "Failed to parse message: Protobuf wire format is malformed or corrupt"; }
}

sub merge {
    my ($self, $dst, $src) = @_;
    my $mdef = $dst->{_mdef};

    foreach my $name (keys %{$src->{_fields}}) {
        my $val = $src->{_fields}{$name};
        next unless defined $val;

        my $fdef = $mdef->find_field_by_name($name);
        next unless $fdef;

        if ($fdef->is_map) {
            $dst->{_fields}{$name} ||= bless {}, 'Protobuf::Internal::Map::Public';
            foreach my $k (keys %$val) {
                $dst->{_fields}{$name}{$k} = _deep_clone($val->{$k});
            }
        } elsif ($fdef->is_repeated) {
            $dst->{_fields}{$name} ||= bless [], 'Protobuf::Internal::Repeated::Public';
            foreach my $item (@$val) {
                push @{$dst->{_fields}{$name}}, _deep_clone($item);
            }
        } elsif ($fdef->type_number == 11) {
            if (exists $dst->{_fields}{$name}) {
                $self->merge($dst->{_fields}{$name}, $val);
            } else {
                $dst->{_fields}{$name} = _deep_clone($val);
            }
        } else {
            $dst->{_fields}{$name} = _deep_clone($val);
        }
    }
    return;
}

sub copy {
    my ($self, $dst, $src) = @_;
    $dst->{_fields} = {};
    foreach my $k (keys %{$src->{_fields}}) {
        $dst->{_fields}{$k} = _deep_clone($src->{_fields}{$k});
    }
    return;
}

sub to_json {
    my ($self, $msg) = @_;
    # TODO
    croak "PurePerl JSON encoder not yet implemented";
}

sub from_json {
    my ($self, $class, $json) = @_;
    # TODO
    croak "PurePerl JSON decoder not yet implemented";
}

sub _clean_value {
    my ($val) = @_;
    return unless defined $val;
    if (ref($val)) {
        if (eval { $val->isa('Math::BigInt') }) {
            return $val->numify();
        } elsif (ref($val) eq 'ARRAY') {
            return [ map { _clean_value($_) } @$val ];
        } elsif (ref($val) eq 'HASH') {
            my $res = {};
            while (my ($k, $v) = each %$val) {
                $res->{$k} = _clean_value($v);
            }
            return $res;
        }
    }
    return $val;
}

sub to_perl {
    my ($self, $msg) = @_;
    my $fields = $msg->{_fields};
    my $res = {};

    foreach my $name (keys %$fields) {
        my $val = $fields->{$name};
        if (ref($val)) {
            if ($val->isa('Protobuf::Message')) {
                $res->{$name} = $val->to_perl();
            } elsif ($val->isa('Protobuf::Internal::Repeated::Public')) {
                $res->{$name} = $val->to_perl();
            } elsif (ref($val) eq 'ARRAY') {
                $res->{$name} = [ map { ref($_) && $_->isa('Protobuf::Message') ? $_->to_perl() : $_ } @$val ];
            } elsif (ref($val) eq 'HASH') {
                # Map
                my $map_res = {};
                while (my ($k, $v) = each %$val) {
                    $map_res->{$k} = (ref($v) && $v->isa('Protobuf::Message')) ? $v->to_perl() : $v;
                }
                $res->{$name} = $map_res;
            } else {
                $res->{$name} = $val;
            }
        } else {
            $res->{$name} = $val;
        }
    }
    return _clean_value($res);
}

sub to_text {
    my ($self, $msg, $indent) = @_;
    $indent //= 0;
    my $indent_str = '  ' x $indent;
    my $res = '';

    my $mdef = $msg->{_mdef};
    return '' unless $mdef;

    my $field_count = $mdef->field_count;
    for my $i (0 .. $field_count - 1) {
        my $fdef = $mdef->get_field($i);
        my $name = $fdef->name;

        next unless $self->has($msg, $name);

        my $val = $self->get($msg, $name);
        next unless defined $val;

        if ($fdef->is_map) {
            my $subm = $fdef->message_type;
            foreach my $key (sort keys %$val) {
                $res .= "${indent_str}${name} {\n";
                $res .= "${indent_str}  key: " . _format_scalar_value($subm->find_field_by_name('key'), $key) . "\n";
                $res .= "${indent_str}  value: " . _format_value_for_text($self, $subm->find_field_by_name('value'), $val->{$key}, $indent + 2) . "\n";
                $res .= "${indent_str}}\n";
            }
        } elsif ($fdef->is_repeated) {
            foreach my $item (@$val) {
                $res .= _format_field_for_text($self, $fdef, $item, $indent_str, $indent);
            }
        } else {
            $res .= _format_field_for_text($self, $fdef, $val, $indent_str, $indent);
        }
    }

    return $res;
}

sub _format_field_for_text {
    my ($self, $fdef, $val, $indent_str, $indent) = @_;
    my $name = $fdef->name;
    my $type = $fdef->type_number;

    if ($type == 11) { # MESSAGE
        my $sub_text = $self->to_text($val, $indent + 1);
        return "${indent_str}${name} {\n${sub_text}${indent_str}}\n";
    } else {
        my $fmt = _format_scalar_value($fdef, $val);
        return "${indent_str}${name}: ${fmt}\n";
    }
}

sub _format_value_for_text {
    my ($self, $fdef, $val, $indent) = @_;
    my $type = $fdef->type_number;
    if ($type == 11) {
        my $sub_text = $self->to_text($val, $indent + 1);
        my $indent_str = '  ' x $indent;
        return "{\n${sub_text}${indent_str}}";
    } else {
        return _format_scalar_value($fdef, $val);
    }
}

sub _format_scalar_value {
    my ($fdef, $val) = @_;
    my $type = $fdef->type_number;
    if ($type == 9 || $type == 12) { # STRING, BYTES
        $val =~ s/\\/\\\\/g;
        $val =~ s/"/\\"/g;
        return "\"$val\"";
    } elsif ($type == 8) { # BOOL
        return $val ? "true" : "false";
    } elsif ($type == 14) { # ENUM
        my $edef = $fdef->enum_type;
        if ($edef) {
            my $ev = $edef->find_value_by_number($val);
            return $ev->name if $ev;
        }
        return $val;
    }
    return $val;
}

sub _deep_clone {
    my ($val) = @_;
    return $val unless defined $val;
    my $ref = ref($val);
    return $val unless $ref;

    if ($ref eq 'ARRAY') {
        return [ map { _deep_clone($_) } @$val ];
    } elsif ($ref eq 'HASH') {
        return { map { $_ => _deep_clone($val->{$_}) } keys %$val };
    } elsif (eval { $val->isa('Protobuf::Internal::Map::Public') }) {
        my $new_map = bless {}, $ref;
        foreach my $k (keys %$val) {
            $new_map->{$k} = _deep_clone($val->{$k});
        }
        return $new_map;
    } elsif (eval { $val->isa('Protobuf::Internal::Repeated::Public') }) {
        my $new_arr = bless [], $ref;
        foreach my $item (@$val) {
            push @$new_arr, _deep_clone($item);
        }
        return $new_arr;
    } elsif (eval { $val->isa('Math::BigInt') }) {
        return $val->copy;
    } elsif (eval { $val->isa('Protobuf::Message') }) {
        my $new_msg = $ref->new(profile => 'pure_perl');
        foreach my $k (keys %{$val->{_fields}}) {
            $new_msg->{_fields}{$k} = _deep_clone($val->{_fields}{$k});
        }
        if (exists $val->{_unknown_fields}) {
            $new_msg->{_unknown_fields} = $val->{_unknown_fields};
        }
        return $new_msg;
    }
    return $val;
}

sub THAW {
    my ($self) = @_;
    return $self;
}

1;
