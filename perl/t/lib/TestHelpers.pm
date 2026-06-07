package TestHelpers;

use strict;
use warnings;
use Protobuf::DescriptorPool;
use Protobuf::ClassGenerator;
use Test::More;
use Exporter 'import';

our @EXPORT = qw(vdiag vnote);

sub vdiag {
    my (@args) = @_;
    if ($ENV{VERBOSE_TESTS}) {
        diag(@args);
    }
}

sub vnote {
    my (@args) = @_;
    if ($ENV{VERBOSE_TESTS}) {
        note(@args);
    }
}

=head1 NAME

TestHelpers - Centralized helper functions for Protobuf Perl tests

=head1 SYNOPSIS

    use lib 't/lib';
    use TestHelpers;

    my $pool = TestHelpers->get_generated_pool();
    my $empty_pool = TestHelpers->get_empty_pool();

    TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

=cut

sub get_generated_pool {
    my $class = shift;
    return Protobuf::DescriptorPool->generated_pool();
}

sub get_empty_pool {
    my $class = shift;
    return Protobuf::DescriptorPool->new();
}

sub load_test_protos {
    my ($class, $pool, @files) = @_;

    # Handle arrayref if passed as first arg after $pool
    if (@files == 1 && ref($files[0]) eq 'ARRAY') {
        @files = @{$files[0]};
    }

    my $last_result;
    foreach my $file_path (@files) {
        open my $fh, '<:raw', $file_path or die "Could not open '$file_path': $!";
        my $data = do { local $/; <$fh> };
        close $fh;
        $last_result = $pool->add_serialized_file_descriptor_set($data);
    }

    return $last_result;
}

sub generate_classes {
    my ($class, $pool) = @_;
    # Find the FileDef for 'test.proto'
    my $file = $pool->find_file_by_name('t/c/test.proto') || $pool->find_file_by_name('test.proto');
    if ($file) {
        Protobuf::ClassGenerator->generate_for_file($file);
    } else {
        # Try to find a message and get its file
        my $mdef = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
        if ($mdef) {
            foreach my $name ('protobuf_perl_test.TestMessage', 'protobuf_perl_test.NestedMessage') {
                my $m = $pool->find_message_by_name($name);
                Protobuf::ClassGenerator->generate_for_message($m) if $m;
            }
        }
 else {
            die "Could not find any descriptors in pool to generate classes";
        }
    }
}

1;
