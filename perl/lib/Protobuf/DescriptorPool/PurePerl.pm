package Protobuf::DescriptorPool::PurePerl;

use strict;
use warnings;
use Protobuf::Engine::PurePerl::DescriptorParser;
use Protobuf::Descriptor::File::PurePerl;
use Protobuf::Descriptor::MessageDef::PurePerl;
use Protobuf::Descriptor::FieldDef::PurePerl;
use Protobuf::Descriptor::EnumDef::PurePerl;
use Protobuf::Descriptor::EnumValueDef::PurePerl;
use Protobuf::Descriptor::OneofDef::PurePerl;
use Carp qw(croak);

sub new {
    my $class = shift;
    return bless {
        files => {},
        messages => {},
        enums => {},
        services => {},
    }, $class;
}

sub add_serialized_file {
    my ($self, $data) = @_;
    my $f_data = Protobuf::Engine::PurePerl::DescriptorParser->parse_file_descriptor($data);
    my $file_obj = $self->_add_file_data($f_data);
    $self->_resolve_all();
    return $file_obj;
}

sub add_serialized_file_descriptor_set {
    my ($self, $data) = @_;
    my $files_data = Protobuf::Engine::PurePerl::DescriptorParser->parse_file_descriptor_set($data);
    my $results = [];
    foreach my $f_data (@$files_data) {
        push @$results, $self->_add_file_data($f_data);
    }

    $self->_resolve_all();

    return $results;
}

sub _resolve_all {
    my ($self) = @_;
    foreach my $mdef (values %{$self->{messages}}) {
        foreach my $fdef (@{$mdef->{_data}{fields}}) {
            my $type_name = $fdef->{_data}{_type_name};
            next unless $type_name;

            # Try to resolve as message
            my $msg = $self->find_message_by_name($type_name);
            if ($msg) {
                $fdef->{_data}{message_type} = $msg;
            } else {
                # Try to resolve as enum
                my $enum = $self->find_enum_by_name($type_name);
                if ($enum) {
                    $fdef->{_data}{enum_type} = $enum;
                }
            }
        }
    }
}

sub _add_file_data {
    my ($self, $f_data) = @_;

    my $pkg = $f_data->{package};
    my $prefix = $pkg ? "$pkg." : "";

    my $file_obj = Protobuf::Descriptor::File::PurePerl->new({
        name => $f_data->{name},
        package => $pkg,
        message_types => [],
        enum_types => [],
    });

    foreach my $m_data (@{$f_data->{message_type} || []}) {
        my $m_obj = $self->_add_message_data($m_data, $prefix, $file_obj);
        push @{$file_obj->{_data}{message_types}}, $m_obj;
    }

    foreach my $e_data (@{$f_data->{enum_type} || []}) {
        my $e_obj = $self->_add_enum_data($e_data, $prefix, $file_obj);
        push @{$file_obj->{_data}{enum_types}}, $e_obj;
    }

    $self->{files}{$f_data->{name}} = $file_obj;
    return $file_obj;
}

sub _add_message_data {
    my ($self, $m_data, $prefix, $file_obj) = @_;
    my $full_name = $prefix . $m_data->{name};

    my $m_obj = Protobuf::Descriptor::MessageDef::PurePerl->new({
        name => $m_data->{name},
        full_name => $full_name,
        file => $file_obj,
        fields => [],
        oneofs => [],
        nested_types => [],
        enum_types => [],
        options => $m_data->{options} || {},
    });

    if (exists $self->{messages}{$full_name}) {
        croak("Symbol already defined in this DescriptorPool: $full_name");
    }
    $self->{messages}{$full_name} = $m_obj;

    # Build oneofs first
    my @oneof_objs;
    my $oneof_decl = $m_data->{oneof_decl} || [];
    for my $i (0 .. $#$oneof_decl) {
        my $o_data = $oneof_decl->[$i];
        my $o_obj = Protobuf::Descriptor::OneofDef::PurePerl->new({
            name => $o_data->{name},
            index => $i,
            containing_type => $m_obj,
            fields => [],
        });
        push @oneof_objs, $o_obj;
    }
    $m_obj->{_data}{oneofs} = \@oneof_objs;

    foreach my $f_data (@{$m_data->{field} || []}) {
        my $f_obj = Protobuf::Descriptor::FieldDef::PurePerl->new({
            name => $f_data->{name},
            number => $f_data->{number},
            type => $f_data->{type},
            label => $f_data->{label},
            full_name => "$full_name." . $f_data->{name},
            _type_name => $f_data->{type_name},
            containing_oneof => undef,
        });

        my $oi = $f_data->{oneof_index};
        if (defined $oi) {
            my $o_obj = $oneof_objs[$oi];
            if ($o_obj) {
                $f_obj->{_data}{containing_oneof} = $o_obj;
                push @{$o_obj->{_data}{fields}}, $f_obj;
            }
        }
        push @{$m_obj->{_data}{fields}}, $f_obj;
    }

    my $inner_prefix = "$full_name.";
    foreach my $n_data (@{$m_data->{nested_type} || []}) {
        my $n_obj = $self->_add_message_data($n_data, $inner_prefix, $file_obj);
        push @{$m_obj->{_data}{nested_types}}, $n_obj;
    }

    foreach my $e_data (@{$m_data->{enum_type} || []}) {
        my $e_obj = $self->_add_enum_data($e_data, $inner_prefix, $file_obj);
        push @{$m_obj->{_data}{enum_types}}, $e_obj;
    }

    return $m_obj;
}

sub _add_enum_data {
    my ($self, $e_data, $prefix, $file_obj) = @_;
    my $full_name = $prefix . $e_data->{name};

    my $e_obj = Protobuf::Descriptor::EnumDef::PurePerl->new({
        name => $e_data->{name},
        full_name => $full_name,
        file => $file_obj,
        values => [],
    });

    if (exists $self->{enums}{$full_name}) {
        croak("Symbol already defined in this DescriptorPool: $full_name");
    }
    $self->{enums}{$full_name} = $e_obj;

    foreach my $v_data (@{$e_data->{value} || []}) {
        my $v_obj = Protobuf::Descriptor::EnumValueDef::PurePerl->new({
            name => $v_data->{name},
            number => $v_data->{number},
            full_name => "$full_name." . $v_data->{name},
        });
        push @{$e_obj->{_data}{values}}, $v_obj;
    }

    return $e_obj;
}

sub _normalize_name {
    my ($name) = @_;
    return undef unless defined $name;
    $name =~ s/^\.//;
    return $name;
}

sub find_file_by_name {
    my ($self, $name) = @_;
    return $self->{files}{$name};
}

sub find_message_by_name {
    my ($self, $name) = @_;
    $name = _normalize_name($name);
    return $self->{messages}{$name};
}

sub find_enum_by_name {
    my ($self, $name) = @_;
    $name = _normalize_name($name);
    return $self->{enums}{$name};
}

1;
