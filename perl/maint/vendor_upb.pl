#!/usr/bin/env perl

use strict;
use warnings;
use File::Spec;
use File::Path qw(make_path);
use File::Copy qw(copy);
use File::Find;
use Cwd qw(abs_path);
use HTTP::Tiny;

my $perl_dir = abs_path(File::Spec->curdir());
my $project_root = abs_path(File::Spec->catdir($perl_dir, '..'));

my $vendor_dir = File::Spec->catdir($perl_dir, 'vendor');

# Check if we are in a Google3 dev environment (where upb is a sibling to protobuf)
my $is_google3 = -d abs_path(File::Spec->catdir($perl_dir, '..', '..', 'upb'));

my $upb_src_dir = $is_google3
    ? abs_path(File::Spec->catdir($perl_dir, '..', '..', 'upb'))
    : File::Spec->catdir($project_root, 'upb');

my $upb_generator_src_dir = $is_google3
    ? abs_path(File::Spec->catdir($perl_dir, '..', '..', 'upb', 'upb_generator'))
    : File::Spec->catdir($project_root, 'upb_generator');

my $utf8_range_src_dir = $is_google3
    ? abs_path(File::Spec->catdir($perl_dir, '..', '..', 'utf8_range'))
    : File::Spec->catdir($project_root, 'third_party', 'utf8_range');

my $proto_src_dir = $is_google3
    ? abs_path(File::Spec->catdir($perl_dir, '..', '..', '..', 'google', 'protobuf'))
    : File::Spec->catdir($project_root, 'src', 'google', 'protobuf');

print "Vendoring upb and third_party into $vendor_dir...\n";
if ($is_google3) {
    print "Google3 environment detected. Sourcing from sibling directories.\n";
}

# Function to recursively copy files matching a pattern and post-process them
sub recursive_copy {
    my ($src_root, $dst_root, $pattern, $post_process, $ignore_pattern) = @_;

    if (!-d $src_root) {
        die "Source directory does not exist: $src_root";
    }

    find({
        wanted => sub {
            return if $ignore_pattern && $File::Find::name =~ $ignore_pattern;
            return unless /$pattern$/;
            return if -d $_;

            my $rel_path = File::Spec->abs2rel($_, $src_root);
            # Map Google3 stage0 paths to open-source paths for CPAN compatibility
            $rel_path =~ s{stage0/3rd_party/protobuf}{stage0/google/protobuf}g;
            $rel_path =~ s{stage0/third_party/protobuf}{stage0/google/protobuf}g;
            # Map 3rd_party to third_party for CPAN compatibility
            $rel_path =~ s/\b3rd_party\b/third_party/g;
            my $dst_path = File::Spec->catfile($dst_root, $rel_path);

            my ($vol, $dir, $file) = File::Spec->splitpath($dst_path);
            make_path(File::Spec->catpath($vol, $dir, ''));

            if ($post_process) {
                # Read, process, and write to dest
                open(my $fh_in, '<', $_) or die "Cannot open for reading: $_: $!";
                my $content = do { local $/; <$fh_in> };
                close($fh_in);

                $content = $post_process->($content, $rel_path);

                open(my $fh_out, '>', $dst_path) or die "Cannot open for writing: $dst_path: $!";
                print $fh_out $content;
                close($fh_out);
            } else {
                copy($_, $dst_path) or die "Copy failed: $_ -> $dst_path: $!";
            }
        },
        no_chdir => 1,
    }, $src_root);
}

my $upb_post_process = sub {
    my ($content, $rel_path) = @_;

    if ($rel_path && $rel_path =~ /descriptor_bootstrap\.h$/) {
        return <<'EOF';
#ifndef THIRD_PARTY_UPB_UPB_REFLECTION_DESCRIPTOR_BOOTSTRAP_H_
#define THIRD_PARTY_UPB_UPB_REFLECTION_DESCRIPTOR_BOOTSTRAP_H_

#ifdef GOOGLE3
#include "net/proto2/proto/descriptor.upb.h"
#else
#include "google/protobuf/descriptor.upb.h"
#endif

#endif  // THIRD_PARTY_UPB_UPB_REFLECTION_DESCRIPTOR_BOOTSTRAP_H_
EOF
    }

    # Surgical Copybara stripping! Remove all blocks wrapped in copybara:strip_begin/end.
    # Must run BEFORE other replacements to avoid prefix-mapping modified code.
    $content =~ s/\/\/\s*copybara:strip_begin.*?copybara:strip_end[^\n]*\n?//gs;

    # Strip "third_party/upb/" and "third_party/utf8_range/" from includes
    $content =~ s/#include\s+["']third_party\/upb\/(.*?)["']/#include "$1"/g;
    $content =~ s/#include\s+["']third_party\/utf8_range\/(.*?)["']/#include "$1"/g;

    # Map third_party/protobuf/ or 3rd_party/protobuf/ to google/protobuf/ (preserving any prefix like upb_generator/stage0/)
    $content =~ s/#include\s+["'](.*?)(?:3rd_party|third_party)\/protobuf\/(.*?)["']/#include "$1google\/protobuf\/$2"/g;

    # Simulate Copybara stripping for Google3-specific markers (like UPB_IS_GOOGLE3)
    $content =~ s/#define\s+UPB_IS_GOOGLE3.*?\n//g;

    # Map Google3 native descriptor includes back to open-source stage0 includes for CPAN compatibility
    if (!$is_google3) {
        $content =~ s/#include\s+["'](?:upb\/reflection\/stage\d\/)?net\/proto2\/proto\/descriptor\.upb\.h["']/#include "google\/protobuf\/descriptor.upb.h"/g;
    } else {
        $content =~ s/#include\s+["'](?:upb\/reflection\/stage\d\/)?net\/proto2\/proto\/descriptor\.upb\.h["']/#include "net\/proto2\/proto\/descriptor.upb.h"/g;
    }

    # Map Google3-specific proto2 prefixes back to open-source google_protobuf prefixes
    # Handle double underscores first (e.g. proto2__ -> google__protobuf__)
    $content =~ s/\bproto2__/google__protobuf__/g;
    # Handle single underscores (e.g. proto2_ -> google_protobuf_)
    $content =~ s/\bproto2_/google_protobuf_/g;

    return $content;
};

my $utf8_post_process = sub {
    my ($content, $rel_path) = @_;
    # Strip "third_party/utf8_range/" from includes
    $content =~ s/#include\s+["']third_party\/utf8_range\/(.*?)["']/#include "$1"/g;
    return $content;
};

my $proto_post_process = sub {
    my ($content, $rel_path) = @_;

    # Strip Copybara blocks: remove all blocks wrapped in copybara:strip_begin/end.
    $content =~ s/\/\/\s*copybara:strip_begin.*?copybara:strip_end[^\n]*\n?//gs;

    $content =~ s{\/\/\s*copybara:insert_begin\n(.*?)\/\/\s*copybara:insert_end[^\n]*\n?}{do {
        my $insert_content = $1;
        $insert_content =~ s/^\/\/ ?//gm; # Remove leading '// ' from each line
        $insert_content;
    }}gse;

    # Strip any remaining Google-internal options that open-source protoc doesn't understand
    $content =~ s/option\s+(java_mutable_api|java_api_version|java_alt_api_package)\s*=\s*[^;]+;//g;
    return $content;
};

# 1. Vendor upb (all headers and sources)
# In Google3, we skip upb/reflection/stage0 because we JIT-copy the open-source stage0
# headers from github/stage0 in Step 1c, and the Google3 descriptor from blaze-bin in Step 1d.
# This prevents conflicts between the stage0 Google3 descriptor and the newly built one.
my $upb_ignore_pattern = $is_google3 ? qr{/upb/reflection/stage0/} : undef;

recursive_copy(
    $upb_src_dir,
    File::Spec->catdir($vendor_dir, 'upb'),
    qr/\.(c|h|inc|upb\.c|upb\.h)$/,
    $upb_post_process,
    $upb_ignore_pattern
);

# 1b. Copy generated upb_edition_defaults.h from blaze-bin if in Google3
if ($is_google3) {
    my $blaze_gen_hdr = File::Spec->catfile(
        abs_path(File::Spec->catdir($perl_dir, '..', '..', '..')),
        'blaze-bin', 'third_party', 'upb', 'upb', 'reflection', 'internal', 'upb_edition_defaults.h'
    );
    my $dest_gen_hdr = File::Spec->catfile(
        $vendor_dir, 'upb', 'upb', 'reflection', 'internal', 'upb_edition_defaults.h'
    );

    if (-f $blaze_gen_hdr) {
        print "Copying generated upb_edition_defaults.h from blaze-bin...\n";
        make_path(File::Spec->catpath((File::Spec->splitpath($dest_gen_hdr))[0,1], ''));
        copy($blaze_gen_hdr, $dest_gen_hdr) or die "Copy failed: $blaze_gen_hdr -> $dest_gen_hdr: $!";
    } else {
        warn "WARNING: Generated header not found at $blaze_gen_hdr.\n";
        warn "Please run: blaze build //third_party/upb/upb/reflection:embedded_upb_edition_defaults_generate\n";
    }
}

# 1c. JIT copy the open-source stage0 headers from github/stage0 (always copy so local make test_c works in CitC)
{
    my $github_stage0_dir = abs_path(File::Spec->catdir(
        $perl_dir, '..', '..', '..', 'third_party', 'upb', 'github', 'stage0', 'google', 'protobuf'
    ));
    my $dest_stage0_dir = File::Spec->catdir(
        $vendor_dir, 'upb', 'upb', 'reflection', 'stage0', 'google', 'protobuf'
    );

    if (-d $github_stage0_dir) {
        print "Copying open-source stage0 headers from github/stage0...\n";
        recursive_copy($github_stage0_dir, $dest_stage0_dir, qr/\.(c|h)$/, $upb_post_process);
    } else {
        warn "WARNING: GitHub stage0 directory not found at $github_stage0_dir.\n";
    }
}

# 1d. Copy generated descriptor.upb.h/c from third_party/upb/upb/reflection/stage0 if in Google3
if ($is_google3) {
    my $upb_stage0_proto_dir = abs_path(File::Spec->catdir(
        $perl_dir, '..', '..', '..', 'third_party', 'upb', 'upb', 'reflection', 'stage0', 'net', 'proto2', 'proto'
    ));
    my $dest_proto_dir = File::Spec->catdir(
        $vendor_dir, 'upb', 'net', 'proto2', 'proto'
    );
    my $dest_proto_dir_os = File::Spec->catdir(
        $vendor_dir, 'upb', 'google', 'protobuf'
    );

    my @files = (
        'descriptor.upb.h',
        'descriptor.upb.c',
    );

    make_path($dest_proto_dir);
    make_path($dest_proto_dir_os);

    foreach my $file (@files) {
        my $src = File::Spec->catfile($upb_stage0_proto_dir, $file);

        if (-f $src) {
            print "Copying Google3 stage0 descriptor file $file...\n";
            open(my $fh_in, '<', $src) or die "Cannot open for reading: $src: $!";
            my $content = do { local $/; <$fh_in> };
            close($fh_in);

            my $content_processed = $upb_post_process->($content);

            # Write to Google3 path
            my $dst = File::Spec->catfile($dest_proto_dir, $file);
            open(my $fh_out, '>', $dst) or die "Cannot open for writing: $dst: $!";
            print $fh_out $content_processed;
            close($fh_out);

            # Write to Open Source path (only the header!)
            if ($file =~ /\.h$/) {
                my $dst_os = File::Spec->catfile($dest_proto_dir_os, $file);
                open(my $fh_out_os, '>', $dst_os) or die "Cannot open for writing: $dst_os: $!";
                print $fh_out_os $content_processed;
                close($fh_out_os);
            }
        } else {
            die "REQUIRED Google3 stage0 descriptor file not found at $src. Has third_party/upb been modified?\n";
        }
    }
}

# 1e. Copy pcre2.h from third_party/pcre2/src/pcre2.h to vendor/third_party/pcre2/pcre2.h in Google3
# This physically recreates the virtual Bazel include path "third_party/pcre2/pcre2.h" so it resolves under make.
if ($is_google3) {
    my $pcre2_src = abs_path(File::Spec->catfile(
        $perl_dir, '..', '..', '..', 'third_party', 'pcre2', 'src', 'pcre2.h'
    ));
    my $dest_pcre2_dir = File::Spec->catdir(
        $vendor_dir, 'third_party', 'pcre2'
    );
    my $dest_pcre2_file = File::Spec->catfile($dest_pcre2_dir, 'pcre2.h');

    if (-f $pcre2_src) {
        print "Copying Google3 pcre2.h to vendor layout...\n";
        make_path($dest_pcre2_dir);
        copy($pcre2_src, $dest_pcre2_file) or die "Copy failed: $pcre2_src -> $dest_pcre2_file: $!";
    } else {
        die "REQUIRED Google3 pcre2.h not found at $pcre2_src. Has third_party/pcre2 been modified?\n";
    }
}

# 1f. Copy generated toolchain.h from blaze-bin if in Google3
# This is required because Abseil's policy_checks.h includes it, but it is a generated header
# that is not present in the source tree, breaking local make builds.
if ($is_google3) {
    my $blaze_gen_hdr = File::Spec->catfile(
        $perl_dir, '..', '..', '..', 'blaze-genfiles', 'third_party', 'absl', 'base', 'google', 'toolchain.h'
    );
    my $dest_gen_hdr = File::Spec->catfile(
        $vendor_dir, 'third_party', 'absl', 'base', 'google', 'toolchain.h'
    );

    if (-f $blaze_gen_hdr) {
        print "Copying generated toolchain.h from blaze-genfiles...\n";
        make_path(File::Spec->catpath((File::Spec->splitpath($dest_gen_hdr))[0,1], ''));
        copy($blaze_gen_hdr, $dest_gen_hdr) or die "Copy failed: $blaze_gen_hdr -> $dest_gen_hdr: $!";
    } else {
        warn "WARNING: Generated Abseil toolchain.h not found at $blaze_gen_hdr.\n";
        warn "Please run: blaze build //third_party/absl/base/google:toolchain_enforcement\n";
    }
}

# 1g. Copy Crubit annotations.h from third_party/crubit/ if in Google3
# This is required because Abseil's cord.h includes it as "crubit/support/annotations.h",
# but it is located under third_party/crubit/ in the source tree, breaking local make builds.
if ($is_google3) {
    my $crubit_src = abs_path(File::Spec->catfile(
        $perl_dir, '..', '..', '..', 'third_party', 'crubit', 'support', 'annotations.h'
    ));
    my $dest_crubit_dir = File::Spec->catdir(
        $vendor_dir, 'crubit', 'support'
    );
    my $dest_crubit_file = File::Spec->catfile($dest_crubit_dir, 'annotations.h');

    if (-f $crubit_src) {
        print "Copying Google3 Crubit annotations.h to vendor layout...\n";
        make_path($dest_crubit_dir);
        copy($crubit_src, $dest_crubit_file) or die "Copy failed: $crubit_src -> $dest_crubit_file: $!";
    } else {
        die "REQUIRED Google3 Crubit annotations.h not found at $crubit_src.\n";
    }
}

# 1h. Copy stage0 plugin.upb.h from upb_generator to the open-source path under vendor/google/protobuf/
# This is required because our compiler plugin (compiled in open-source mode to avoid missing generated headers)
# includes it as "google/protobuf/compiler/plugin.upb.h", but the JIT-vendored stage0 header is located
# under third_party/protobuf/compiler/ in upb_generator, breaking local make builds.
if ($is_google3) {
    my $stage0_hdr = File::Spec->catfile(
        $upb_generator_src_dir, 'stage0', '3rd_party', 'protobuf', 'compiler', 'plugin.upb.h'
    );
    my $dest_gen_hdr = File::Spec->catfile(
        $vendor_dir, 'google', 'protobuf', 'compiler', 'plugin.upb.h'
    );

    if (-f $stage0_hdr) {
        print "Copying stage0 plugin.upb.h to vendor layout (with post-processing)...\n";
        make_path(File::Spec->catpath((File::Spec->splitpath($dest_gen_hdr))[0,1], ''));

        open(my $fh_in, '<', $stage0_hdr) or die "Cannot open for reading: $stage0_hdr: $!";
        my $content = do { local $/; <$fh_in> };
        close($fh_in);

        # Run post-process to rename proto2 -> google_protobuf
        $content = $upb_post_process->($content, 'google/protobuf/compiler/plugin.upb.h');

        open(my $fh_out, '>', $dest_gen_hdr) or die "Cannot open for writing: $dest_gen_hdr: $!";
        print $fh_out $content;
        close($fh_out);
    } else {
        die "REQUIRED JIT stage0 plugin.upb.h not found at $stage0_hdr. Has third_party/upb been modified?\n";
    }
}

# 2. Vendor upb_generator (needed for reflection/stage0)
recursive_copy(
    $upb_generator_src_dir,
    File::Spec->catdir($vendor_dir, 'upb_generator'),
    qr/\.(c|h|inc|upb\.c|upb\.h)$/,
    $upb_post_process
);

# 3. Vendor third_party/utf8_range
recursive_copy(
    $utf8_range_src_dir,
    File::Spec->catdir($vendor_dir, 'third_party', 'utf8_range'),
    qr/(utf8_range\.c|utf8_range\.h|utf8_validity\.h|.*\.inc)$/,
    $utf8_post_process
);

# 4. Vendor src/google/protobuf (protos)
recursive_copy(
    $proto_src_dir,
    File::Spec->catdir($vendor_dir, 'src', 'google', 'protobuf'),
    qr/\.proto$/,
    $proto_post_process
);

if ($is_google3) {
    # In Google3, the test protos are at the root of third_party/protobuf,
    # but the C tests expect them in the open-source src/google/protobuf/ layout.
    # We also copy them to third_party/protobuf/ to match Google3 import paths in test.proto.
    my $dst_dir_src = File::Spec->catdir($vendor_dir, 'src', 'google', 'protobuf');
    my $dst_dir_g3 = File::Spec->catdir($vendor_dir, 'third_party', 'protobuf');

    foreach my $file ('test_messages_proto2.proto', 'test_messages_proto3.proto') {
        my $src = File::Spec->catfile($project_root, $file);
        if (-f $src) {
            print "Copying Google3 test proto $file to vendor layout...\n";
            open(my $fh_in, '<', $src) or die "Cannot open for reading: $src: $!";
            my $content = do { local $/; <$fh_in> };
            close($fh_in);

            # Strip Google-internal options that open-source protoc doesn't understand
            $content = $proto_post_process->($content, $file);

            # Write to open-source layout
            make_path($dst_dir_src);
            my $dst_src = File::Spec->catfile($dst_dir_src, $file);
            open(my $fh_out_src, '>', $dst_src) or die "Cannot open for writing: $dst_src: $!";
            print $fh_out_src $content;
            close($fh_out_src);

            # Write to Google3 layout in vendor
            make_path($dst_dir_g3);
            my $dst_g3 = File::Spec->catfile($dst_dir_g3, $file);
            open(my $fh_out_g3, '>', $dst_g3) or die "Cannot open for writing: $dst_g3: $!";
            print $fh_out_g3 $content;
            close($fh_out_g3);
        } else {
            warn "WARNING: Google3 test proto $file not found at $src.\n";
        }
    }
}

# 5. JIT fetch libcoro (C coroutine library for testing)
# We download it at build time to avoid checking in non-Apache licensed code.
my $libcoro_vendor_dir = File::Spec->catdir($vendor_dir, 'libcoro');
print "JIT fetching libcoro into $libcoro_vendor_dir...\n";
make_path($libcoro_vendor_dir);

my $http = HTTP::Tiny->new();
my @coro_files = ('coro.c', 'coro.h', 'LICENSE');
my $coro_base_url = 'https://raw.githubusercontent.com/semistrict/libcoro/master/';

foreach my $file (@coro_files) {
    my $url = $coro_base_url . $file;
    my $dest_path = File::Spec->catfile($libcoro_vendor_dir, $file);

    print "  Downloading $url -> $dest_path...\n";
    my $response = $http->get($url);

    if ($response->{success}) {
        open(my $fh, '>', $dest_path) or die "Cannot open for writing: $dest_path: $!";
        print $fh $response->{content};
        close($fh);
    } else {
        warn "WARNING: Failed to download $file from $url: $response->{status} $response->{reason}\n" .
             "C-level coroutine safety tests will be skipped or fail to compile.\n";
    }
}

# 6. Generate vendor/.clang-format dynamically to disable lint formatting
my $clang_format_path = File::Spec->catfile($vendor_dir, '.clang-format');
print "Generating $clang_format_path...\n";
make_path($vendor_dir);
open(my $fh_cf, '>', $clang_format_path) or die "Cannot open for writing: $clang_format_path: $!";
print $fh_cf <<'EOF';
# Generated by vendor_upb.pl. Do not edit.
# Disable formatting for JIT-vendored third-party code.
DisableFormat: true
OverridePriorities: true
EOF
close($fh_cf);

print "Done.\n";
