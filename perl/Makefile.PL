use strict;
use warnings;
use lib 'local/lib';
use lib 'xlib';
use ExtUtils::MakeMaker;
use Config;
use File::Spec;
use File::Find;
use ExtUtils::Embed;
use JSON::MaybeXS;
use Template;
use Sideload::Build::FileLists qw(
    get_upb_c_files
    get_utf8_c_files
    get_generated_c_files
    get_xs_helper_c_files
);

# --- Path Macros ---
use Cwd qw(abs_path getcwd);
my $cwd = getcwd();

# --- Dev Environment Detection & Sourcing ---
# We detect dev mode relative to the current 'perl/' directory
my $is_google3_dev = (-f 'maint/generate_xs.pl' && -d '../../upb');
my $is_dev = (-f 'maint/generate_xs.pl' && (-d '../upb' || $is_google3_dev));
our $is_darwin = $^O eq 'darwin';
our $is_linux  = $^O eq 'linux';
our $is_win32  = $^O eq 'MSWin32';
our $num_cores = 1;
if ($is_linux) {
    $num_cores = qx(nproc 2>/dev/null) || 1;
} elsif ($is_darwin) {
    $num_cores = qx(sysctl -n hw.ncpu 2>/dev/null) || 1;
} elsif ($is_win32) {
    $num_cores = $ENV{NUMBER_OF_PROCESSORS} || 1;
}
chomp($num_cores);
$num_cores = int($num_cores) || 1;

my $perl = $ENV{PERL} || $^X;

if (!$is_google3_dev && !-d 'vendor') {
    print "vendor/ directory is missing. Bootstrapping upb...\n";
    system($^X, 'maint/vendor_upb.pl') == 0 or die "Failed to bootstrap upb: $!";
}

if ($is_dev) {
    print "Development environment detected.\n";
    if ($is_google3_dev) {
        print "Google3 dev environment detected. Sourcing from sibling directories.\n";
    }
    
    # 0. Ensure the protoc symlink points to the correct compiler plugin source
    my $symlink_name = 'protoc';
    my $target_dir = $is_google3_dev ? '../compiler/perl' : '../src/google/protobuf/compiler/perl';
    my $abs_target_dir = File::Spec->rel2abs($target_dir);

    if (-e $symlink_name && ! -l $symlink_name) {
        warn "WARNING: $symlink_name exists but is a physical directory/file, not a symlink! Please remove it manually if you want to use the dev symlink.\n";
    } elsif (eval { symlink('', ''); 1 }) {
        if (-l $symlink_name) {
            my $current_target = readlink($symlink_name);
            if (!$current_target || $current_target ne $abs_target_dir) {
                print "Correcting symlink $symlink_name (points to " . ($current_target || 'nothing') . ", should point to $abs_target_dir)...\n";
                unlink $symlink_name or die "Failed to remove old symlink $symlink_name: $!";
                symlink($abs_target_dir, $symlink_name) or die "Failed to create symlink $symlink_name -> $abs_target_dir: $!";
            }
        } else {
            print "Creating symlink $symlink_name -> $abs_target_dir...\n";
            symlink($abs_target_dir, $symlink_name) or die "Failed to create symlink $symlink_name -> $abs_target_dir: $!";
        }
    } else {
        warn "WARNING: symlink() is not supported on this platform. You may need to manually map $target_dir to $symlink_name.\n";
    }
    
    # 1. Reassemble Protobuf.xs from fragments
    print "Reassembling Protobuf.xs...\n";
    system($perl, 'maint/generate_xs.pl') == 0 or die "Failed to run generate_xs.pl: $!";

    # 2. Update the local vendor directory with core components
    if (-f 'maint/vendor_upb.pl') {
        print "Updating vendor/ directory...\n";
        system($perl, 'maint/vendor_upb.pl') == 0 or die "Failed to run vendor_upb.pl: $!";
    }

    # 3. Clean and regenerate MANIFEST from scratch to remove dead references
    require ExtUtils::Manifest;
    print "Regenerating MANIFEST...\n";
    unlink 'MANIFEST';
    ExtUtils::Manifest::mkmanifest();
}
# --- End Dev Environment Automation ---

# Now that vendor/ is guaranteed to exist on disk (either because it was already there,
# or because we just generated it above), we ALWAYS define the project root as 'vendor'
# for the build phase. This ensures we compile entirely against clean, stripped headers
# and avoid any header shadowing (e.g., endian.h) or Google3 include prefix issues.
my $raw_root = -d 'vendor' ? 'vendor' : '..';
our $project_root = File::Spec->abs2rel(abs_path($raw_root), $cwd);
our $using_vendor = ($raw_root eq 'vendor');

our $upb_dir = File::Spec->catdir($project_root, 'upb');
our $upb_generator_dir = File::Spec->catdir($project_root, 'upb_generator');
our $src_dir = File::Spec->catdir($project_root, 'src');
our $third_party_dir = File::Spec->catdir($project_root, 'third_party');
our $libcoro_dir = File::Spec->catdir($project_root, 'libcoro');

# --- Create Google3-style import mapping for public builds ---
# test.proto imports "third_party/protobuf/test_messages_proto2.proto".
# In the public build, this file is located at 'vendor/src/google/protobuf/test_messages_proto2.proto'.
# We dynamically create 'vendor/third_party/protobuf/' and symlink/copy the file there
# so that protoc (running with --proto_path=vendor) can resolve the import.
my $vendor_dir = 'vendor';
if (-d $vendor_dir) {
    my $target_dir = File::Spec->catdir($vendor_dir, 'third_party', 'protobuf');
    my $source_file = File::Spec->catfile($vendor_dir, 'src', 'google', 'protobuf', 'test_messages_proto2.proto');

    if (-f $source_file) {
        print "Creating Google3-style import mapping for test messages...\n";
        require File::Path;
        File::Path::make_path($target_dir) unless -d $target_dir;

        my $target_file = File::Spec->catfile($target_dir, 'test_messages_proto2.proto');
        my $abs_source_file = File::Spec->rel2abs($source_file);
        if (!-f $target_file) {
            # Try symlink first, fall back to copy (for Windows compatibility)
            eval { symlink($abs_source_file, $target_file) }
                or do {
                    require File::Copy;
                    File::Copy::copy($source_file, $target_file) or warn "Failed to copy proto file: $!";
                };
        }
    }
}

if (!$is_google3_dev) {
    print "Creating Google3-compatibility symlinks for C++ plugin...\n";
    my $legacy_symlink = 'third_party/protobuf/perl';
    if (-l $legacy_symlink) {
        print "Removing legacy circular symlink $legacy_symlink...\n";
        unlink $legacy_symlink or warn "Failed to remove $legacy_symlink: $!";
    }

    require File::Path;
    File::Path::make_path('third_party/protobuf/perl') unless -d 'third_party/protobuf/perl';

    my $symlink_target = 'third_party/protobuf/perl/protoc';
    if (-e $symlink_target || -l $symlink_target) {
        unlink $symlink_target or warn "Failed to remove old symlink $symlink_target: $!";
    }

    my $abs_protoc_dir = File::Spec->rel2abs('protoc');
    eval { symlink($abs_protoc_dir, $symlink_target) }
        or do {
            warn "WARNING: Failed to create symlink $symlink_target -> $abs_protoc_dir: $!\n" .
                 "C++ plugin compilation might fail due to missing Google3-style headers.\n";
        };
}

our $bazel_gen_include = $is_google3_dev
    ? File::Spec->catdir($upb_dir, "upb", "reflection", "stage0")
    : File::Spec->catdir($upb_dir, "reflection", "stage0");
our $cmake_gen_include = $is_google3_dev
    ? File::Spec->catdir($upb_dir, "upb", "reflection", "cmake")
    : File::Spec->catdir($upb_dir, "reflection", "cmake");

our $our_protoc_path = 'protoc';
if ($is_darwin) {
    if (-x '/opt/homebrew/bin/protoc') {
        $our_protoc_path = '/opt/homebrew/bin/protoc';
    } elsif (-x '/usr/local/bin/protoc') {
        $our_protoc_path = '/usr/local/bin/protoc';
    }
}
print "Auto-detected protoc path: $our_protoc_path\n";
# --- End Path Macros ---
# --- End Dev Environment Automation ---



# Template Toolkit helper function
my $tt;
sub _render_tt {
    my ($block_name, $vars) = @_;
    $vars ||= {};
    $vars->{PROTOC} ||= $our_protoc_path;
    $vars->{is_win32} = $is_win32;
    $vars->{is_darwin} = $is_darwin;
    if (!$tt) {
        $tt = Template->new({
            INCLUDE_PATH => [ 'inc/templates' ],
            PRE_PROCESS  => 'makefile.tt',
            PRE_CHOMP    => 0,
            POST_CHOMP   => 0,
            TRIM         => 0,
        });
    }
    my $output = '';
    my $template_string = "[% PROCESS $block_name %]";
    if (!$tt->process(\$template_string, $vars, \$output)) {
        die "Template processing failed for block $block_name: " . $tt->error();
    }
    return $output;
}

sub _get_sanitizer_flags {
    my $asan_flags = $ENV{USE_ASAN} ? "-fsanitize=address" : "";
    my $ubsan_flags = $ENV{USE_UBSAN} ? "-fsanitize=undefined" : "";
    my $msan_flags = $ENV{USE_MSAN} ? "-fsanitize=memory" : "";
    my $tsan_flags = $ENV{USE_TSAN} ? "-fsanitize=thread" : "";
    return join(" ", grep { $_ } $asan_flags, $ubsan_flags, $msan_flags, $tsan_flags);
}

my $sanitizer_flags = _get_sanitizer_flags();

# Attempt to generate ppport.h
eval { require Devel::PPPort; };
if (!$@) {
    warn "Generating ppport.h ...$/";
    if (!Devel::PPPort::WriteFile('ppport.h')) {
        warn "Failed to generate ppport.h: $!";
    }
} else {
    warn main::_render_tt('devel_ppport_warning', {});
}

# Define your common C source files relative to perl/
my @UPB_C_FILES = get_upb_c_files($project_root, $is_google3_dev);
my @UTF8_C_FILES = get_utf8_c_files($project_root);
my @GENERATED_C_FILES = get_generated_c_files($project_root, $is_google3_dev);
my @XS_HELPER_C_FILES = get_xs_helper_c_files($project_root);

my @COMMON_C_FILES = (@UPB_C_FILES, @UTF8_C_FILES, @GENERATED_C_FILES, @XS_HELPER_C_FILES);

# Pure UPB object files for the protoc plugin (no Perl dependencies)
my @PURE_UPB_O_FILES = grep { $_ !~ /plugin\.upb/ } map {
    my $o = $_;
    $o =~ s{^\Q$project_root\E[\\/]}{};
    $o =~ s/\.cc?$/$Config{obj_ext}/;
    $o;
} (@UPB_C_FILES, @UTF8_C_FILES, @GENERATED_C_FILES);
my $PURE_UPB_O_STRING = join(' ', @PURE_UPB_O_FILES);

# Mangle object file names to avoid collisions and place them in the current dir (perl/)
my @COMMON_O_FILES = map {
    my $o = $_;
    $o =~ s{^\Q$project_root\E[\\/]}{}; # Remove the relative root prefix
    $o =~ s/\.cc?$/$Config{obj_ext}/;
    $o;
} @COMMON_C_FILES;

our $extlib_ext = $is_darwin ? 'dylib' : '$(DLEXT)';
my $COMMON_LIB = "libprotobufperl.$extlib_ext";
my $COMMON_O_STRING = join(' ', @COMMON_O_FILES);

my @COMMON_FILES_PAIRS;
for my $i (0 .. $#COMMON_C_FILES) {
    push @COMMON_FILES_PAIRS, { c => $COMMON_C_FILES[$i], o => $COMMON_O_FILES[$i] };
}

# Subroutines to override MakeMaker behavior must be defined before WriteMakefile

sub _get_adjusted_embed_flags {
    use ExtUtils::Embed;
    use File::Spec;
    use Config;

    my $embed_cflags = ExtUtils::Embed::ccopts(); # CC flags for embedding
    my $embed_ldflags = ExtUtils::Embed::ldopts(); # LD flags for embedding

    # Remove potential newlines
    chomp $embed_cflags;
    chomp $embed_ldflags;

    # Replace -lperl with the actual path to libperl
    my $archlib = $Config{archlib};
    my $libperl = $Config{libperl};
    my $perl_lib_path = File::Spec->catfile($Config{archlibexp}, 'CORE', $libperl);
    if (!-f $perl_lib_path) {
        my ($lib_base) = $archlib =~ m{^(.+)/perl/};
        $perl_lib_path = File::Spec->catfile($lib_base, $libperl);
    }
    $embed_ldflags =~ s{-lperl}{$perl_lib_path}g;

    return ($embed_cflags, $embed_ldflags);
}

# Rules to build the static library and its object files
sub MY::postamble {
    my $self = shift;
    use Config;
    use File::Spec;
    use JSON::MaybeXS;
    use Path::Tiny;

    # Common library build rules
    print "MY::postamble: is_google3_dev = ", ($is_google3_dev ? "TRUE" : "FALSE"), "\n";
    print "MY::postamble: bazel_gen_include = $bazel_gen_include\n";
    print "MY::postamble: cmake_gen_include = $cmake_gen_include\n";
    my $common_o_string = $COMMON_O_STRING;
    my $common_lib = $COMMON_LIB;
    my $file_pairs = \@COMMON_FILES_PAIRS;

    my $cxx = $main::is_win32 ? 'g++' : ($ENV{CXX} || 'g++');
    my $make_frag = "CXX = $cxx\n\n";
    $make_frag .= main::_render_tt('common_lib_def', { COMMON_LIB => $common_lib });
    $make_frag .= $/;



    my %done_o;
    my @obj_rules;
    # .... OBJECT FILE RULES for libprotobufperl.a ....
    foreach my $pair (@$file_pairs) {
        my $c_file = $pair->{c};
        my $o_file = $pair->{o};
        next if $done_o{$o_file}++;
        my $compiler = '$(CC)';
        # Using global $bazel_gen_include and $cmake_gen_include
        my $bootstrap_define = "-DUPB_BOOTSTRAP_STAGE=0";
        my $base_flags = "-I. -I$project_root -I$upb_dir -I$src_dir -I$third_party_dir -I" . File::Spec->catdir($third_party_dir, 'utf8_range') . " -I$bazel_gen_include -I$cmake_gen_include $bootstrap_define";

        my $cflags = "";
        if ($c_file =~ /\.cc$/) {
            $compiler = '$(CXX)';
            $cflags = "-Wno-error=attributes -I" . File::Spec->catdir($third_party_dir, 'abseil-cpp');
        } else {
            $cflags = "-D_GNU_SOURCE -Werror=incompatible-pointer-types -Wno-error=attributes -std=c99 -Wall -Wstrict-prototypes";
        }
        push @obj_rules, main::_render_tt('common_obj_rule', {
            c => $c_file,
            o => $o_file,
            compiler => $compiler,
            cflags => $cflags,
            base_flags => $base_flags,
            deps => '',
        });
    }
    $make_frag .= join("", @obj_rules) . $/;

    $make_frag .= main::_render_tt('all_common_lib_dep', { COMMON_LIB => $common_lib });
    $make_frag .= $/;

    $make_frag .= main::_render_tt('common_lib_link_rule', {
        COMMON_LIB => $common_lib,
        COMMON_O_STRING => $common_o_string,
        is_linux => $main::is_linux,
        common_lib_link_flags => $main::common_lib_link_flags,
    });
    $make_frag .= $/;

    # Rule for upb-perl-test.o
    $make_frag .= main::_render_tt('upb_perl_test_o', {
        deps => '',
    });
    $make_frag .= $/;

    # Custom C test rules
    my ($embed_cflags, $embed_ldflags) = main::_get_adjusted_embed_flags();
    my $archlibexp = $Config{archlibexp};

    my $config_file = File::Spec->catfile('t', 'c', 'c_test_config.json');
    my $json_text = Path::Tiny::path($config_file)->slurp_utf8;

    # Replace hardcoded monorepo paths with dynamic path macros
    # In Google3, the dependencies have an extra 'upb/' level
    my $upb_dep_dir = File::Spec->catdir($upb_dir, "upb");

    $json_text =~ s{\.\./upb/}{\$upb_dep_dir/}g;
    $json_text =~ s{\.\./src/}{\$src_dir/}g;
    $json_text =~ s{\.\./third_party/}{\$third_party_dir/}g;

    # Actually, it's better to just replace them with the actual paths
    # because we are about to decode the JSON.
    # We convert backslashes to forward slashes to avoid JSON escaping issues on Windows.
    my $upb_dep_dir_esc = $upb_dep_dir; $upb_dep_dir_esc =~ s{\\}{/}g;
    my $src_dir_esc = $src_dir; $src_dir_esc =~ s{\\}{/}g;
    my $third_party_dir_esc = $third_party_dir; $third_party_dir_esc =~ s{\\}{/}g;

    $json_text =~ s{\$upb_dep_dir}{$upb_dep_dir_esc}g;
    $json_text =~ s{\$src_dir}{$src_dir_esc}g;
    $json_text =~ s{\$third_party_dir}{$third_party_dir_esc}g;

    my $c_test_config = decode_json($json_text);


    my @c_test_targets;
    my $c_test_rules = "# c test rules$/";
    my $test_inc = '$(INC)'; # Inherit INC from WriteMakefile

    foreach my $test_key ( sort keys %$c_test_config ){
        next if $test_key =~ /eval_croak_test/;
        if ($test_key =~ /coro/) {
            if ($Config{archname} !~ /x86_64|i[3456]86/) {
                warn "Skipping coroutine test $test_key on non-x86 architecture ($Config{archname})$/";
                next;
            }
            if (!-f 'vendor/libcoro/coro.h') {
                warn "Skipping coroutine test $test_key because vendor/libcoro/coro.h is missing (JIT fetch failed or skipped)$/";
                next;
            }
        }

        my $target = "t/c/$test_key";
        my $source = "$target.c";
        my $object = "$target.o";
        $target .= ".exe" if $main::is_win32;
        push @c_test_targets, $target;

        my $config = $c_test_config->{$test_key} || {};
        my @extra_src_list;
        my $extra_src_val = $config->{extra_src};
        if (defined $extra_src_val) {
            push @extra_src_list, ref $extra_src_val eq 'ARRAY' ? @{$extra_src_val} : $extra_src_val;
        }
        my $deps = join(' ', @{$config->{deps} || []});
        my $ccflags = $config->{ccflags} || '';
        my $ldflags = $config->{ldflags} || '';
        my $extra_ccflags = $config->{extra_ccflags} || '';

        my @all_objects = ($object);
        my @all_sources = ($source);
        push @all_sources, @extra_src_list;

        foreach my $src_file (@extra_src_list) {
            my $obj_file = $src_file;
            $obj_file =~ s/\.c$/.o/;
            push @all_objects, $obj_file;
        }
        my $all_objects_str = join(" ", @all_objects);

        # Filter all_sources to avoid duplicates with COMMON_C_FILES
        my @test_sources;
        foreach my $s (@all_sources) {
            my $is_common = 0;
            foreach my $common_c (@COMMON_C_FILES) {
                if ($s eq $common_c) {
                    $is_common = 1;
                    last;
                }
            }
            push @test_sources, $s unless $is_common;
        }

        # Rule to compile all .c/.cc to .o for this test
        foreach my $s (@test_sources) {
            my $o = $s;
            $o =~ s/\.cc?$/.o/;
            next if $done_o{$o}++;
            my $compiler = ($s =~ /\.cc$/) ? '$(CXX)' : '$(CC)';
            $c_test_rules .= main::_render_tt('c_test_one_compile', {
                s => $s,
                o => $o,
                deps => $deps,
                dev_deps => '',
                compiler => $compiler,
                embed_cflags => $embed_cflags,
                test_inc => $test_inc,
                extra_ccflags => $extra_ccflags,
                space => ' ',
            });
        }

        # Rule to link the object files into an executable
        my $test_deps = "$all_objects_str \$(COMMON_LIB) t/c/upb-perl-test.o";
        if ($target eq 't/c/convert/sv_to_upb' || $target eq 't/c/convert/upb_to_sv' || $target eq 't/c/integration/050_convert') {
            $test_deps .= " t/data/test_descriptor.bin";
        }

        my $link_line = join " ", grep { length $_ > 0 } (
            '$(CC)', '$(CCFLAGS)', $extra_ccflags, '-o', $target, $all_objects_str,
            't/c/upb-perl-test.o', '-L$(CURDIR)', '-lprotobufperl',
            $embed_cflags, $embed_ldflags, $ldflags, "-lpcre2-8 $sanitizer_flags"
        );

        $c_test_rules .= main::_render_tt('c_test_one_link', {
            target => $target,
            test_deps => $test_deps,
            link_line => $link_line,
        });
    }
    $c_test_rules .= "# end c test rules$/";
    $make_frag .= $c_test_rules;

    # C Benchmarks rules
    my @bench_sources = glob("t/c/benchmark/*.c");
    my @c_bench_targets;
    my @benchmark_params = map {
        my $target = $_;
        $target =~ s/\.c$//;
        my $exe_target = $target . ($main::is_win32 ? ".exe" : "");
        push @c_bench_targets, $exe_target;
        { source => $_, target => $exe_target, object => "$target.o" }
    } @bench_sources;

    $make_frag .= main::_render_tt('c_benchmark_rules', {
        benchmarks => \@benchmark_params,
        embed_cflags => $embed_cflags,
        embed_ldflags => $embed_ldflags,
        sanitizer_flags => $sanitizer_flags,
    });

    # Add all C test and benchmark targets to .PHONY
    $make_frag .= main::_render_tt('phony', {
        c_test_targets => \@c_test_targets,
        c_bench_targets => \@c_bench_targets,
    });

    # Rule to generate test_descriptor.bin
    $make_frag .= main::_render_tt('test_descriptor_bin', {
        project_root => $project_root,
    });

    # Target to build all C tests & benchmarks
    $make_frag .= main::_render_tt('build_aggregator_targets', {
        c_test_targets  => \@c_test_targets,
        c_bench_targets => \@c_bench_targets,
    });
    $make_frag .= $/;

    # Rules to run all C tests and benchmarks
    $make_frag .= main::_render_tt('run_targets', {
        c_test_targets  => \@c_test_targets,
        c_bench_targets => \@c_bench_targets,
        archlibexp      => $Config{archlibexp},
        COMMON_LIB => $common_lib,
    });
    $make_frag .= $/;

    # Rule for TSAN test
    $make_frag .= main::_render_tt('tsan_test', {});
    $make_frag .= $/;

    # Rule for protoc-gen-perl-pb executable
    my @plugin_srcs = (
        "protoc/plugin_main.cc",
        "protoc/generator.cc",
        "protoc/helpers.cc",
    );
    my @plugin_objs = map { my $obj = $_; $obj =~ s/\.cc$/\.o/; $obj } @plugin_srcs;
    my $protoc_plugin_exe = "protoc-gen-perl-pb";
    my $plugin_upb_c = ($is_google3_dev && !$using_vendor)
        ? File::Spec->catfile($upb_generator_dir, "stage0", "third_party", "protobuf", "compiler", "plugin.upb.c")
        : File::Spec->catfile($upb_generator_dir, "stage0", "google", "protobuf", "compiler", "plugin.upb.c");
    my $plugin_upb_o = "protoc/plugin.upb.o";
    my $descriptor_upb_c;
    if ($is_google3_dev && !$using_vendor) {
        $descriptor_upb_c = File::Spec->catfile($upb_dir, "net", "proto2", "proto", "descriptor.upb.c");
    } elsif ($using_vendor) {
        $descriptor_upb_c = File::Spec->catfile($upb_dir, "upb", "reflection", "stage0", "google", "protobuf", "descriptor.upb.c");
    } else {
        $descriptor_upb_c = File::Spec->catfile($upb_dir, "reflection", "stage0", "google", "protobuf", "descriptor.upb.c");
    }
    my $descriptor_upb_o = "protoc/descriptor.upb.o";

    my $bootstrap_define = $is_google3_dev ? "" : "-DUPB_BOOTSTRAP_STAGE=0";
    my $protoc_cflags = "-g -D_GNU_SOURCE -std=c++20 -Wall $bootstrap_define";
    my $g3_inc = $is_google3_dev ? "-I../../.." : "";
    my $protoc_inc = "-I. -I./protoc -I$project_root -I$upb_dir -I$src_dir -I$third_party_dir -I" . File::Spec->catdir($third_party_dir, 'utf8_range') . " -I" . $bazel_gen_include . " -I" . $cmake_gen_include . " -I" . File::Spec->catdir($upb_generator_dir, 'stage0') . " -I$Config{archlibexp}/CORE $g3_inc $main::mac_inc";

    foreach my $i (0 .. $#plugin_srcs) {
        $make_frag .= main::_render_tt('protoc_plugin_compile', {
            src => $plugin_srcs[$i],
            obj => $plugin_objs[$i],
            cflags => $protoc_cflags,
            inc => $protoc_inc,
        });
        $make_frag .= $/;
    }

    # Rules for upb plugin objects
    $make_frag .= "$plugin_upb_o: $plugin_upb_c\n\t\$(CC) -I. -I$project_root -I$upb_dir -I" . File::Spec->catdir($upb_generator_dir, 'stage0') . " -I" . $bazel_gen_include . " -O2 -c $plugin_upb_c -o $plugin_upb_o\n\n";

    my $absl_libs = "";
    unless ($is_win32) {
        $absl_libs = "-labsl_strings -labsl_throw_delegate -labsl_base -labsl_int128";
        $absl_libs .= " -labsl_string_view" unless $is_darwin;
    }
    $make_frag .= main::_render_tt('protoc_plugin_link', {
        exe => $protoc_plugin_exe,
        obj => join(" ", @plugin_objs) . " $plugin_upb_o",
        common_lib => $COMMON_LIB,
        common_o => $PURE_UPB_O_STRING,
        absl_libs => $absl_libs,
        sanitizer_flags => $sanitizer_flags,
    });
    $make_frag .= $/;

    # Rules to build the protoc plugin using Bazel
    $make_frag .= main::_render_tt('bazel_protoc_plugin', {});
    $make_frag .= $/;

    # Copy libprotobufperl to blib so it can be found via Rpath without LD_LIBRARY_PATH
    my $ext = $main::extlib_ext;
    $make_frag .= <<"EOF";

# Make the shared library a dependency of the blib copy
\$(INST_ARCHAUTODIR)/libprotobufperl.$ext : libprotobufperl.$ext
	\$(NOECHO) \$(MKPATH) \$(INST_ARCHAUTODIR)
	\$(CP) libprotobufperl.$ext \$(INST_ARCHAUTODIR)/libprotobufperl.$ext

# Ensure the copy happens during the build
all :: \$(INST_ARCHAUTODIR)/libprotobufperl.$ext

EOF

    return $make_frag;

    }

    # Add bazel_build_protoc_plugin to the default dependencies
    sub MY::all_before {
    return '';
    }

my @clean_targets = (
    'blib',
    'third_party',
    "libprotobufperl.$extlib_ext",
    'ppport.h',
    'protoc/plugin_main.o',
    'protoc/generator.o',
    'protoc/helpers.o',
    'protoc/plugin.upb.o',
    'protoc/descriptor.upb.o',
    'protoc-gen-perl-pb',
    @COMMON_O_FILES,
    't/c/upb-perl-test.o',
);
push @clean_targets, 'vendor' if $is_google3_dev;
my %clean_files_map = map { $_ => 1 } @clean_targets;

# Add C test binaries and their objects
if (-f 't/c/c_test_config.json') {
    require JSON::MaybeXS;
    require Path::Tiny;
    my $c_test_config = JSON::MaybeXS::decode_json(Path::Tiny::path('t/c/c_test_config.json')->slurp_utf8);
    foreach my $test_key (keys %$c_test_config) {
        $clean_files_map{"t/c/$test_key"} = 1;
        $clean_files_map{"t/c/$test_key.o"} = 1;
        my $config = $c_test_config->{$test_key} || {};
        if (my $extra_src = $config->{extra_src}) {
            my @extras = ref $extra_src eq 'ARRAY' ? @$extra_src : ($extra_src);
            foreach my $src (@extras) {
                my $obj = $src;
                $obj =~ s/\.cc?$/.o/;
                $clean_files_map{$obj} = 1;
            }
        }
    }
}
# Add benchmark binaries
foreach my $t (glob("t/c/benchmark/*.c")) {
    $t =~ s/\.c$//;
    $clean_files_map{$t} = 1;
    $clean_files_map{"$t.o"} = 1;
}

our $common_lib_link_flags = '';
our $extra_ldflags = '';
our $mac_inc = '';
our $mac_libs = '';
if ($is_linux) {
    $common_lib_link_flags = '-Wl,--version-script=libprotobufperl.map';
    $extra_ldflags = " -Wl,-rpath,'\$\$ORIGIN'";
} elsif ($is_darwin) {
    $common_lib_link_flags = "-Wl,-install_name,\@rpath/libprotobufperl.$extlib_ext -Wl,-undefined,dynamic_lookup";
    $extra_ldflags = ' -Wl,-rpath,@loader_path';
    if (-d '/opt/homebrew/include') {
        $mac_inc = ' -I/opt/homebrew/include';
        $mac_libs = ' -L/opt/homebrew/lib';
    }
}

my @clean_files = sort keys %clean_files_map;

WriteMakefile(
    NAME              => 'Protobuf',
    VERSION           => '0.05',
    ABSTRACT          => 'High-performance Protocol Buffers implementation using upb',
    PREREQ_PM         => {
      'Moo'             => 0,
      'Type::Tiny'      => 0,
      'Types::Standard' => 0,
      'Log::Any'        => 0,
      'JSON::MaybeXS'   => 0,
      'Const::Fast'     => 0,
      'Math::BigInt'    => 0,
      'MIME::Base64'    => 0,
    },
    META_MERGE => {
        'meta-spec' => { version => 2 },
        resources => {
            repository => {
                type => 'git',
                url  => 'https://github.com/LLC-Technologies-Collier/protobuf.git',
                web  => 'https://github.com/LLC-Technologies-Collier/protobuf',
            },
        },
    },
    TEST_REQUIRES => {
      'Test::More'       => 0,
      'Test::MockModule' => 0,
      'File::Spec'       => 0,
      'Path::Tiny'       => 0,
      'Capture::Tiny'    => 0,
    },
    CONFIGURE_REQUIRES => {
        'ExtUtils::MakeMaker' => 0,
        'Config'              => 0,
        'File::Spec'          => 0,
        'File::Find'          => 0,
        'ExtUtils::Embed'     => 0,
        'JSON::MaybeXS'       => 0,
        'Template'            => 0,
        'Path::Tiny'          => 0,
        'Cwd'                 => 0,
        'Devel::PPPort'       => 0,
    },
    AUTHOR            => 'C.J. Collier <cjac@colliertech.org>',
    LICENSE           => 'apache_2_0',
    DEFINE            => '-DUPB_BOOTSTRAP_STAGE=0' . ($is_google3_dev ? ' -DGOOGLE3' : ''),
    INC               => "-I. -I$project_root -I$upb_dir -I$src_dir -I$third_party_dir -I$libcoro_dir -I" . File::Spec->catdir($third_party_dir, 'utf8_range') . " -I$bazel_gen_include -I$cmake_gen_include -I\$(PERL_INC)$mac_inc",
    MYEXTLIB          => "libprotobufperl.$extlib_ext",
    CCFLAGS           => "-g $Config{ccflags} -fPIC $sanitizer_flags",
    LIBS              => ['-L$(CURDIR) -lprotobufperl' . $mac_libs],
    LDDLFLAGS         => "$Config{lddlflags} $sanitizer_flags$extra_ldflags$mac_libs",
    LDFLAGS           => "$Config{ldflags} $sanitizer_flags$extra_ldflags$mac_libs",

    # Clean the generated object files and the static library
    clean             => { FILES => join(' ', sort @clean_files) },
    realclean         => { FILES => 'vendor upb Protobuf-*.tar.gz lua ruby perl local MANIFEST MANIFEST.bak make.log test_c.log Protobuf.xs BUILD.bazel META.json META.yml' },

    dist              => {
        PREOP => '$(PERL) maint/vendor_upb.pl && $(PERL) -MExtUtils::Manifest=mkmanifest -e "mkmanifest"',
    },
);

package MY;

# Modify the MY::test subroutine
sub test {
    my $self = shift;
    return main::_render_tt('my_test', {
        is_darwin => $main::is_darwin,
        jobs      => $main::num_cores,
    });
}

package main;
1;
