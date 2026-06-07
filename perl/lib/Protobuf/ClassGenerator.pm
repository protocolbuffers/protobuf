=encoding UTF-8

=head1 NAME

Protobuf::ClassGenerator - Generates Perl classes from Protocol Buffer descriptors

=head1 VERSION

version 0.05

=head1 SYNOPSIS

    use Protobuf::ClassGenerator;
    use Protobuf::DescriptorPool;

    my $pool = Protobuf::DescriptorPool->generated_pool;
    my $file_def = $pool->find_file_by_name('my/app/protos.proto');

    # Generate classes for all messages in the file
    Protobuf::ClassGenerator->generate_for_file($file_def);

    # Now you can use the generated classes
    # my $msg = My::App::Message->new();

=head1 DESCRIPTION

This module is responsible for dynamically creating Perl classes based on Protocol Buffer message descriptors (L<Protobuf::Descriptor::MessageDef>). When a C<.proto> file is loaded into a L<Protobuf::DescriptorPool>, this generator is invoked to build the corresponding Moo-based Perl classes, complete with accessors and other methods for each field.

Key features of the generated classes:

=over 4

=item * Inherit from L<Protobuf::Message>.

=item * Have methods for getting, setting, checking presence, and clearing each field (e.g., C<my_field()>, C<set_my_field()>, C<has_my_field()>, C<clear_my_field()>).

=item * Return tied arrays/hashes for repeated/map fields, offering a standard Perl interface.

=item * Associate the class with its L<Protobuf::Descriptor::MessageDef>.

=back

This module is primarily used internally by L<Protobuf::DescriptorPool> when adding new file descriptors.

=head1 METHODS

=head2 generate_for_file($file_descriptor)

Takes a L<Protobuf::Descriptor::File> object and generates Perl classes for all top-level and nested messages defined within that file. This method is called recursively for nested types.

=head2 generate_for_message($message_descriptor)

Generates the Perl class for a single L<Protobuf::Descriptor::MessageDef>.

=head2 generate_type_library($file_descriptor)

(Experimental) Generates a string containing code for a L<Type::Library> based on the message definitions in the given L<Protobuf::Descriptor::File>.

=head2 type_library($file_descriptor)

Alias for C<generate_type_library>.

=head2 generate_docs()

(Placeholder) Intended to generate HTML documentation from descriptors.

=head2 generate_validator_xs($message_descriptor)

(Experimental) Generates C code for a highly optimized validator function for the given message type.

=head1 SEE ALSO

L<Protobuf>, L<Protobuf::DescriptorPool>, L<Protobuf::Message>, L<Protobuf::Descriptor>

=head1 AUTHOR

C.J. Collier <cjac@google.com>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2026 by Google LLC.

This is free software; you can redistribute it and/or modify it under
the terms of the BSD 3-Clause License.

=cut

package Protobuf::ClassGenerator;

use strict;
use warnings;
use Protobuf;
# Protobuf::Internal is only needed for XS features
if ($Protobuf::HAS_XS) {
    require Protobuf::Internal;
    Protobuf::Internal->import(':all');
}
use Carp qw(croak);
use Log::Any qw($log);
use Protobuf::Descriptor::EnumValue;

our %DESCRIPTOR_REGISTRY;
our %FIELD_REGISTRY;
our %EXTENSION_RANGES;
our %GENERATED;

my %CUSTOM_CAPITALIZATION = (
    'bigquery' => 'BigQuery',
    'pubsub'   => 'PubSub',
);

sub _capitalize_segment {
    my ($segment) = @_;
    return $CUSTOM_CAPITALIZATION{lc($segment)} // ucfirst($segment);
}

sub register_extension_range {
    my ($class, $full_msg_name, $start, $end) = @_;
    push @{$EXTENSION_RANGES{$full_msg_name}}, { start => $start, end => $end };
    return;
}

sub generate_for_file {
    my ($class, $file) = @_;
    return unless $file;

    my $proto_file = $file->name;
    $proto_file =~ s/.*\///; # Basename
    $proto_file =~ s/\..*//; # Remove extension
    my $file_module = join('', map { _capitalize_segment($_) } split(/_/, $proto_file));

    my $pkg = $file->get_package;
    my $base_module = join('::', map { _capitalize_segment($_) } split(/\./, $pkg));
    $base_module .= "::$file_module" if $base_module;
    $base_module ||= $file_module;

    $log->debugf("Generating classes for file: %s (Base: %s)", $file->name, $base_module);

    my $count = $file->top_level_message_count;
    for my $i (0 .. $count - 1) {
        my $mdef = $file->get_top_level_message($i);
        _generate_recursively($mdef, $base_module);
    }

    my $enum_count = $file->top_level_enum_count;
    for my $i (0 .. $enum_count - 1) {
        my $edef = $file->get_top_level_enum($i);
        my $full_name = $edef->full_name;
        my $normalized = $full_name;
        $normalized =~ s/^\.//;

        my $perl_class = "${base_module}::" . $edef->name;
        $log->debugf("Generating top-level enum: %s", $perl_class);

        my $vcount = $edef->value_count;
        if ($vcount > 0) {
            my $first_val = $edef->get_value(0)->name;
            no strict 'refs';
            if (defined &{"${perl_class}::$first_val"}) {
                $log->debugf("Enum %s already generated, skipping", $perl_class);
                next;
            }
        }

        my $code = "package $perl_class;\n";
        for my $j (0 .. $vcount - 1) {
            my $vdef = $edef->get_value($j);
            my $vname = $vdef->name;
            my $vnumber = $vdef->number;
            $code .= "sub $vname { $vnumber }\n";
        }
        $code .= "1;\n";
        {
            no strict 'refs';
            eval $code;
        }
        die "Failed to generate enum $perl_class: $@" if $@;
    }
    return;
}

sub _generate_recursively {
    my ($mdef, $current_ns) = @_;
    return unless $mdef->isa('Protobuf::Descriptor::MessageDef') || $mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl');
    _generate_for_message($mdef, $current_ns);

    my $nested_count = $mdef->nested_message_count;
    for my $i (0 .. $nested_count - 1) {
        my $subm = $mdef->get_nested_message($i);
        _generate_recursively($subm, "${current_ns}::" . $mdef->name);
    }
}

sub generate_type_library {
    my ($class, $file) = @_;

    my $proto_file = $file->name;
    $proto_file =~ s/.*\///;
    $proto_file =~ s/\..*//;
    my $file_module = join('', map { _capitalize_segment($_) } split(/_/, $proto_file));

    my $pkg = $file->get_package;
    my $base_module = join('::', map { _capitalize_segment($_) } split(/\./, $pkg));
    $base_module .= "::$file_module" if $base_module;
    $base_module ||= $file_module;

    my $lib_name = "${base_module}::Types";

    my $code = "package $lib_name;\n";
    $code .= "use Type::Library -base;\n";
    $code .= "use Type::Utils -all;\n";
    $code .= "use Types::Standard -types;\n";
    $code .= "use Protobuf::Message;\n\n";

    my $msg_count = $file->top_level_message_count;
    for my $i (0 .. $msg_count - 1) {
        my $mdef = $file->get_top_level_message($i);
        $code .= _generate_types_recursively($mdef);
    }

    $code .= "1;\n";
    return $code;
}

sub type_library {
    my ($class, $file) = @_;
    return $class->generate_type_library($file);
}

sub _generate_types_recursively {
    my ($mdef) = @_;
    my $full_name = $mdef->full_name;
    my $normalized = $full_name;
    $normalized =~ s/^\.//;
    my $perl_class = join('::', map { _capitalize_segment($_) } split(/\./, $normalized));

    my $type_name = $mdef->name;

    my $code = "declare '$type_name',\n";
    $code .= "    as InstanceOf['$perl_class'],\n";
    $code .= "    where { \$_->validate };\n\n";

    $code .= "coerce '$type_name',\n";
    $code .= "    from HashRef, via { '$perl_class'->from_perl(\$_) };\n\n";

    my $nested_count = $mdef->nested_message_count;
    for my $i (0 .. $nested_count - 1) {
        $code .= _generate_types_recursively($mdef->get_nested_message($i));
    }
    return $code;
}

sub generate_docs {
    my ($class) = @_;
    return "<html><body><h1>Protobuf Documentation</h1></body></html>";
}

sub generate_validator_xs {
    my ($class, $mdef) = @_;
    my $full_name = $mdef->full_name;
    my $perl_class = $mdef->perl_class_name();

    my $normalized = $full_name;
    $normalized =~ s/^\.//;
    my $c_func = "validate_" . $normalized;
    $c_func =~ s/[:\.]/_/g;

    my $code = "/* AOT Validator for $full_name */\n";
    $code .= "bool $c_func(pTHX_ SV* sv) {\n";
    $code .= "    if (!sv || !SvROK(sv)) return false;\n";
    $code .= "    if (!sv_derived_from(sv, \"$perl_class\")) return false;\n";

    $code .= "    HV* hv = (HV*)SvRV(sv);\n";

    my $field_count = $mdef->field_count;
    for my $i (0 .. $field_count - 1) {
        my $f = $mdef->get_field($i);
        if ($f->is_required) {
            my $name = $f->name;
            $code .= "    if (!hv_exists(hv, \"$name\", " . length($name) . ")) return false;\n";
        }
    }

    $code .= "    return true;\n}\n";
    return $code;
}

sub generate_for_message {
    my ($class, $mdef) = @_;
    my $full_name = $mdef->full_name;
    my $normalized = $full_name;
    $normalized =~ s/^\.//;
    my $perl_class = join('::', map { _capitalize_segment($_) } split(/\./, $normalized));
    return _generate_for_message($mdef, $perl_class);
}

sub _generate_for_message {
    my ($mdef, $current_ns) = @_;
    return unless $mdef->isa('Protobuf::Descriptor::MessageDef') || $mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl');

    my $full_name = $mdef->full_name;
    my $normalized = $full_name;
    $normalized =~ s/^\.//;

    my $perl_class = "${current_ns}::" . $mdef->name;
    # If current_ns already ends with message name, don't append it again
    if ($current_ns =~ /::$mdef->name$/) {
        $perl_class = $current_ns;
    }

    # Special handling for Well-Known Types
    require Protobuf::WKT;
    my $ext_class = Protobuf::WKT->get_extension_class($normalized);
    if ($ext_class) {
        eval "require $ext_class";
    }

    # Guard against double-generation
    return if $GENERATED{$perl_class}++;

    # Special handling for Well-Known Types

    $DESCRIPTOR_REGISTRY{$perl_class} = $mdef;
    if ($mdef->isa('Protobuf::Descriptor::MessageDef::PurePerl')) {
        $mdef->{_data}{perl_class} = $perl_class;
    }
    $log->debugf("GENERATING CLASS: %s", $perl_class);

    # Generate the class using string eval
    my $code = '';
    $code .= <<"EOC";
package $perl_class;
use parent 'Protobuf::Message';
use Types::Standard qw(Int Str Num Bool ArrayRef HashRef InstanceOf Any);
use Type::Utils qw(dwim_type);
use Carp qw(croak);

sub descriptor { return \$Protobuf::ClassGenerator::DESCRIPTOR_REGISTRY{'$perl_class'}; }

sub validate {
    my \$self = shift;
    my \$c_func = '_xs_validate_' . '$normalized';
    \$c_func =~ s/[:\\.]/_/g;
    if (\$self->can(\$c_func)) {
        return \$self->\$c_func();
    }
    return \$self->SUPER::validate();
}

EOC

    # Generate enums nested in this message
    my $enum_count = $mdef->nested_enum_count;
    for my $i (0 .. $enum_count - 1) {
        my $edef = $mdef->get_nested_enum($i);
        my $enum_name = $edef->name;
        my $enum_pkg = "${perl_class}::$enum_name";

        my $val_count = $edef->value_count;
        if ($val_count > 0) {
            my $first_val = $edef->get_value(0)->name;
            no strict 'refs';
            if (defined &{"${enum_pkg}::$first_val"}) {
                $log->debugf("Nested enum %s already generated, skipping", $enum_pkg);
                next;
            }
        }

        $log->debugf("Generating nested enum: %s", $enum_pkg);
        $code .= "package $enum_pkg;\n";
        for my $j (0 .. $val_count - 1) {
            my $vdef = $edef->get_value($j);
            my $vname = $vdef->name;
            my $vnumber = $vdef->number;
            $code .= "sub $vname { $vnumber }\n";
        }
    }
    $code .= "package $perl_class;\n";

    my $field_count = $mdef->field_count;
    for my $i (0 .. $field_count - 1) {
        my $fdef = $mdef->get_field($i);
        my $name = $fdef->name;
        $FIELD_REGISTRY{$perl_class}{$name} = $fdef;

        my $type_code = _get_type_tiny_code($fdef);
        my $coercion_code = '';
        my $extra_where = '';
        my $type_num = $fdef->type_number;
        if ($type_num == 11) {
            my $m_type = $fdef->message_type;
            if (!defined $m_type) {
                croak('Message type not resolved for field \'' . $fdef->name . '\' of type \'' . ($fdef->{_data}{_type_name} || 'unknown') . '\'');
            }
            my $m_class = _get_perl_class_for_mdef($m_type);
            if ($fdef->is_repeated) {
                $coercion_code = "->plus_coercions(ArrayRef, sub { my \$arr = \$_; [ map { ref(\$_) eq 'HASH' ? do { my \$m = '$m_class'->new; \$m->from_perl(\$_); \$m } : \$_ } \@\$arr ] })";
            } elsif (!$fdef->is_map) {
                $coercion_code = "->plus_coercions(HashRef, sub { my \$m = '$m_class'->new; \$m->from_perl(\$_); \$m })";
            }
        } elsif ($type_num == 3 || $type_num == 4 || $type_num == 16 || $type_num == 18) {
            # INT64, UINT64, SFIXED64, SINT64
            $coercion_code = "->plus_coercions(Str, sub { Math::BigInt->new(\$_) })";
        }

        if ($type_num == 5 || $type_num == 15 || $type_num == 17) { # INT32, SFIXED32, SINT32
            if ($fdef->is_repeated) {
                $extra_where = '->where(sub { my $arr = $_; !grep { !defined($_) || $_ < -2147483648 || $_ > 2147483647 } @$arr }, message => sub { \'out of range\' })';
            } else {
                $extra_where = '->where(sub { defined($_) && $_ >= -2147483648 && $_ <= 2147483647 }, message => sub { Int()->check($_) ? \'out of range\' : \'did not pass type constraint "Int"\' })';
            }
        } elsif ($type_num == 13 || $type_num == 7) { # UINT32, FIXED32
            if ($fdef->is_repeated) {
                $extra_where = '->where(sub { my $arr = $_; !grep { !defined($_) || $_ < 0 || $_ > 4294967295 } @$arr }, message => sub { \'out of range\' })';
            } else {
                $extra_where = '->where(sub { defined($_) && $_ >= 0 && $_ <= 4294967295 }, message => sub { Int()->check($_) ? \'out of range\' : \'did not pass type constraint "Int"\' })';
            }
        }

        # Perl-level accessors that delegate to the engine
        $code .= <<"EOC";
{
    my \$type = dwim_type("$type_code")$coercion_code$extra_where;
    my \$check = \$type->compiled_check;
    my \$coercion = \$type->coercion;

    sub $name {
        my \$self = shift;
        if (\@_) {
            my \$val = \$_[0];
            if (\$coercion) {
                \$val = \$coercion->coerce(\$val);
            }
            \$check->(\$val) or croak("Invalid value for field '$name' in message '$perl_class': " . \$type->get_message(\$val));
            return \$self->set('$name', \$val);
        }
        return \$self->get('$name');
    }

    sub set_$name {
        my (\$self, \$val) = \@_;
        if (\$coercion) {
            \$val = \$coercion->coerce(\$val);
        }
        \$check->(\$val) or croak("Invalid value for field '$name' in message '$perl_class': " . \$type->get_message(\$val));
        return \$self->set('$name', \$val);
    }
}

sub has_$name {
    my \$self = shift;
    return \$self->has('$name');
}

sub clear_$name {
    my \$self = shift;
    return \$self->clear('$name');
}
EOC

        if ($fdef->is_repeated) {
            $code .= <<"EOC";
sub add_$name {
    my \$self = shift;
    my \$arr = \$self->$name();
    push \@\$arr, \@_;
    return \$arr;
}
EOC
        }
    }

    my $oneof_count = $mdef->oneof_count;
    for my $i (0 .. $oneof_count - 1) {
        my $odef = $mdef->get_oneof($i);
        my $name = $odef->name;
        $code .= <<"EOC";
sub $name {
    my \$self = shift;
    return \$self->which_oneof('$name');
}
EOC
    }

    $code .= "1;\n";

    {
        ## no critic (BuiltinFunctions::ProhibitStringyEval)
        no warnings 'redefine';
        eval $code;
    }
    die "Failed to generate class $perl_class: $@" if $@;

    # Install Fast Accessors (XSUBs) to overwrite the slow Perl-based ones
    if ($Protobuf::HAS_XS) {
        no warnings 'redefine';
        Protobuf::_install_fast_accessors($perl_class, $mdef);
    }

    if ($ext_class) {
        _inject_wkt($perl_class, $ext_class);
    }

    return;
}

sub _inject_wkt {
    my ($perl_class, $ext_class) = @_;
    no strict 'refs';
    if ($ext_class->can('get_injected_methods')) {
        no warnings 'redefine';
        foreach my $method ($ext_class->get_injected_methods()) {
            my $full_sym = "${perl_class}::$method";
            *$full_sym = \&{"${ext_class}::$method"};
        }
    }
}

sub _get_perl_class_for_mdef {
    my ($mdef) = @_;
    my $f = $mdef->file;
    my $perl_class;
    if ($f) {
        my $proto_file = $f->name;
        $proto_file =~ s/.*\///;
        $proto_file =~ s/\..*//;
        my $file_module = join('', map { _capitalize_segment($_) } split(/_/, $proto_file));

        my $pkg = $f->get_package;
        my $base_module = join('::', map { _capitalize_segment($_) } split(/\./, $pkg));
        $base_module .= "::$file_module" if $base_module;
        $base_module ||= $file_module;

        # Now we need the path from the package to the message
        my $full_name = $mdef->full_name;
        $full_name =~ s/^\.//;
        if ($pkg) {
            $full_name =~ s/^\Q$pkg\E\.//;
        }
        my $msg_path = join('::', map { _capitalize_segment($_) } split(/\./, $full_name));
        $perl_class = "${base_module}::${msg_path}";
    } else {
        my $full_name = $mdef->full_name;
        $full_name =~ s/^\.//;
        $perl_class = join('::', map { _capitalize_segment($_) } split(/\./, $full_name));
    }
    return $perl_class;
}

sub _get_type_tiny_code {
    my ($fdef) = @_;
    my $type = $fdef->type_number;
    my $base_type;

    if ($type == 1 || $type == 2) { # DOUBLE, FLOAT
        $base_type = "Num";
    } elsif ($type == 3 || $type == 4 || $type == 16 || $type == 18) {
        # INT64, UINT64, SFIXED64, SINT64
        # Allow Int, Str (for large values), or Math::BigInt objects
        $base_type = "(Int | Str | InstanceOf['Math::BigInt'])";
    } elsif ($type == 5 || $type == 7 || $type == 13 || $type == 15 || $type == 17) {
        # INT32, FIXED32, UINT32, SFIXED32, SINT32
        $base_type = "Int";
    } elsif ($type == 8) { # BOOL
        $base_type = "Bool";
    } elsif ($type == 9 || $type == 12) { # STRING, BYTES
        $base_type = "Str";
    } elsif ($type == 14) { # ENUM
        $base_type = "(Int | Str)";
    } elsif ($type == 11) { # MESSAGE
        my $subm = $fdef->message_type;
        if (!defined $subm) {
            croak('Message type not resolved in _get_type_tiny_code for field \'' . $fdef->name . '\' of type \'' . ($fdef->{_data}{_type_name} || 'unknown') . '\'');
        }
        $base_type = "InstanceOf['" . _get_perl_class_for_mdef($subm) . "']";
    } else {
        return "Any";
    }

    if ($fdef->is_map) {
        # Protobuf maps have restricted key types (usually Str or Int)
        # For simplicity, we just use HashRef for now
        return "HashRef";
    } elsif ($fdef->is_repeated) {
        return "ArrayRef[$base_type]";
    }

    return $base_type;
}

1;
