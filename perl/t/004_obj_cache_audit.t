use strict;
use warnings;
use Test::More;
use Protobuf;
plan skip_all => 'XS required for object cache audit tests' unless $Protobuf::HAS_XS;
use Protobuf::DescriptorPool;
use lib "t/lib";
use TestHelpers;

# Setup pool and load some protos
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

sub dump_log {
    my $msg = shift;
    my $log = Protobuf::Internal::get_cache_audit_log();
    vdiag("--- Audit Log Dump ($msg) ---");
    foreach my $e (@$log) {
        vdiag(sprintf("Type: %d | Ptr: %s | TS: %d", $e->{type}, $e->{ptr}, $e->{timestamp}));
    }
    vdiag("--- End Dump ---");
}

# Clear existing log and cache
Protobuf::Internal::clear_cache();

# 1. ADD event
my $pool1 = Protobuf::DescriptorPool->generated_pool();
my $log = Protobuf::Internal::get_cache_audit_log();

# Look for ADD event for the pool
my @adds = grep { $_->{type} == 1 } @$log;
ok(scalar @adds > 0, 'Log contains ADD event for pool') or dump_log("After generated_pool");

# 2. HIT event
my $pool2 = Protobuf::DescriptorPool->generated_pool();
$log = Protobuf::Internal::get_cache_audit_log();
my @hits = grep { $_->{type} == 2 } @$log;
ok(scalar @hits > 0, 'Log contains HIT event for pool') or dump_log("After second generated_pool");

# 3. MISS event (optional, but generated_pool usually MISSes the first time)
my @misses = grep { $_->{type} == 3 } @$log;
ok(scalar @misses > 0, 'Log contains MISS event for pool') or dump_log("After generated_pool (MISS check)");

# 4. DELETE event
subtest 'DELETE event' => sub {
    Protobuf::Internal::clear_cache();
    my $m = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
    ok($m, "Loaded descriptor");

    my $log_before = Protobuf::Internal::get_cache_audit_log();
    # Find the ADD event
    my ($add_event) = grep { $_->{type} == 1 } @$log_before;
    if (!$add_event) {
        vdiag("No ADD event found in log before delete");
        return;
    }

    my $ptr = $add_event->{ptr};
    vnote("Attempting to delete pointer from log: $ptr");
    Protobuf::Internal::delete_cache_ptr($ptr);

    my $log_after = Protobuf::Internal::get_cache_audit_log();
    # Normalize pointer strings for comparison (remove leading 0x if necessary, etc)
    my $norm_ptr = lc($ptr);
    $norm_ptr =~ s/^0x//;

    my @deletes = grep {
        $_->{type} == 4 && do {
            my $p = lc($_->{ptr});
            $p =~ s/^0x//;
            $p eq $norm_ptr;
        }
    } @$log_after;

    if (!scalar @deletes) {
        vdiag("DELETE event not found for $ptr");
        vdiag("Log after delete attempt:");
        foreach my $e (@$log_after) {
            vdiag("  Type: $e->{type} | Ptr: $e->{ptr}");
        }
    }

    ok(scalar @deletes > 0, "Log contains DELETE event for $ptr");
};

# 5. EVICT event
subtest 'EVICT event' => sub {
    Protobuf::Internal::clear_cache();
    Protobuf::Internal::set_cache_capacity(1);

    # Access multiple distinct descriptors to force eviction
    my $m1 = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
    my $m2 = $pool->find_message_by_name('protobuf_perl_test.NestedMessage');

    my $log = Protobuf::Internal::get_cache_audit_log();
    my @evicts = grep { $_->{type} == 5 } @$log;
    ok(scalar @evicts > 0, 'Log contains EVICT event');
};

# Restore capacity
Protobuf::Internal::set_cache_capacity(100000);

# Check structure of an entry
my $entry = $log->[-1];
ok(exists $entry->{type}, 'Entry has type');
ok(exists $entry->{ptr}, 'Entry has ptr');
ok(exists $entry->{timestamp}, 'Entry has timestamp');

done_testing();
