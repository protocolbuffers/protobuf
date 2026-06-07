package Sideload::Build::FileLists;

use strict;
use warnings;
warn "Loading Sideload::Build::FileLists\n";
use File::Find;
use File::Spec;
use Exporter 'import';

our @EXPORT_OK = qw(
    get_upb_c_files
    get_utf8_c_files
    get_generated_c_files
    get_xs_helper_c_files
);

sub get_upb_c_files {
    my ($project_root, $is_google3) = @_;
    my @files;
    my $dir = File::Spec->catdir($project_root, 'upb');
    return unless -d $dir;
    find(sub {
        my $prune_regex = $is_google3
            ? qr{/(lua|ruby|python|rust|conformance|cmake|stage0|util|js)$}
            : qr{/(lua|ruby|python|rust|conformance|cmake|stage0|util|net|js)$};
            
        if ($File::Find::name =~ $prune_regex) {
            $File::Find::prune = 1;
            return;
        }
        if (/\.c$/ && $File::Find::name !~ /\/test_util\//) {
            push @files, $File::Find::name;
        }
    }, $dir);
    return @files;
}
sub get_utf8_c_files {
    my ($project_root) = @_;
    my @files;
    my $dir = File::Spec->catdir($project_root, 'third_party', 'utf8_range');
    return unless -d $dir;
    # Only include utf8_range.c. Other files are either tests, 
    # stand-alone benchmarks, or included via .inc files.
    push @files, File::Spec->catfile($dir, 'utf8_range.c');
    return @files;
}

sub get_generated_c_files {
    my ($project_root, $is_google3) = @_;
    
    # In Google3, we use the JIT-vendored net/proto2/proto/descriptor.upb.c
    # which is collected by get_upb_c_files. We must NOT compile the open-source
    # stage0 descriptor to avoid duplicate symbol linker conflicts.
    return if $is_google3;
    
    my @files;
    my $dir = File::Spec->catdir($project_root, 'upb', 'upb', 'reflection', 'stage0');
    if (!-d $dir) {
        $dir = File::Spec->catdir($project_root, 'upb', 'reflection', 'stage0');
    }
    return unless -d $dir;
    find(sub {
        if (/\.c$/) {
            push @files, $File::Find::name;
        }
    }, $dir);
    return @files;
}

sub get_xs_helper_c_files {
    my ($project_root) = @_;
    my @files;
    my $dir;
    if (-d 'xs') {
        $dir = 'xs';
    } elsif (defined $project_root) {
        if (-d File::Spec->catdir($project_root, 'perl', 'xs')) {
            $dir = File::Spec->catdir($project_root, 'perl', 'xs');
        } elsif (-d File::Spec->catdir($project_root, 'xs')) {
            $dir = File::Spec->catdir($project_root, 'xs');
        }
    }
    return unless defined $dir && -d $dir;
    find(sub {
        if (/\.c$/) {
            push @files, $File::Find::name;
        }
    }, $dir);
    return @files;
}

1;
