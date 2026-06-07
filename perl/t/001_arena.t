use strict;
use warnings;
use Test::More;
use Protobuf;
plan skip_all => 'XS required for Arena tests' unless $Protobuf::HAS_XS;
use Protobuf::Arena;

subtest 'basic creation' => sub {
    my $arena = Protobuf::Arena->new;
    ok($arena, 'Created arena');
    isa_ok($arena, 'Protobuf::Arena');
    ok($arena->{_arena_ptr}, 'Has internal arena pointer');
};

subtest 'multiple arenas' => sub {
    my $a1 = Protobuf::Arena->new;
    my $a2 = Protobuf::Arena->new;
    isnt($a1->{_arena_ptr}, $a2->{_arena_ptr}, 'Different arenas have different pointers');
};

subtest 'stats method' => sub {
    my $arena = Protobuf::Arena->new;
    my $stats = $arena->stats();
    ok($stats, 'Got stats');
    is(ref($stats), 'HASH', 'Stats is a hash');
    ok(exists $stats->{allocated}, 'Has allocated metric');
    ok(exists $stats->{reserved}, 'Has reserved metric');
    ok(exists $stats->{blocks}, 'Has blocks metric');

    is($arena->space_allocated(), $stats->{allocated}, 'space_allocated() matches stats');
    is($arena->space_reserved(), $stats->{reserved}, 'space_reserved() matches stats');
};

subtest 'cloning' => sub {
    my $arena = Protobuf::Arena->new;
    my $clone = $arena->Clone();
    ok($clone, 'Cloned arena');
    isa_ok($clone, 'Protobuf::Arena');
    isnt($clone->{_arena_ptr}, $arena->{_arena_ptr}, 'Clone has its own arena pointer');
};

subtest 'destruction' => sub {
    {
        my $arena = Protobuf::Arena->new;
    }
    pass('Destroyed arena without crash');
};

subtest 'explicit pointer access' => sub {
    my $arena = Protobuf::Arena->new;
    my $ptr = $arena->{_arena_ptr};
    ok($ptr, "Got pointer $ptr");
};

ok(1, 'All basic tests passed');

TODO: {
    local $TODO = 'Implement Arena Fusion (Deep Cloning) logic';
    ok(0, 'Arena fusion allows data transfer without deep copy');
}

TODO: {
    local $TODO = 'Implement Thread-Local Arena Cache Integration';
    ok(0, 'High-frequency allocation utilizes cached arenas from Perl');
}

TODO: {
    local $TODO = 'Implement NUMA-Aware Arena Allocation API';
    ok(0, 'Perl users can specify CPU/Node affinity for arena placement');
}

# The following are now implemented
subtest 'Arena memory usage statistics' => sub {
    my $arena = Protobuf::Arena->new;
    my $stats = $arena->stats;
    ok($stats->{reserved} > 0, 'Reserved space is positive');
    ok($stats->{allocated} >= 0, 'Allocated space is non-negative');
    is($stats->{blocks}, 1, 'Initial block count is 1');
};

subtest 'Custom Allocators (tmpfs)' => sub {
    use File::Temp qw(tempdir);
    use File::Spec;
    my $tmp = tempdir(CLEANUP => 1);
    my $path = File::Spec->catfile($tmp, "pool_test.shm");
    my $size = 32768;

    my $arena = Protobuf::Arena->new_tmpfs($path, $size);
    ok($arena, 'Created tmpfs arena');
    my $stats = $arena->stats;
    is($stats->{reserved}, $size, 'Reserved matches tmpfs size');
    ok(-f $path, 'Tmpfs file created');
};

done_testing();
