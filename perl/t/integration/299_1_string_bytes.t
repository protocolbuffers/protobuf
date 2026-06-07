use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use Time::HiRes qw(tv_interval gettimeofday);
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/compat_descriptor.bin');

subtest 'basic string and bytes packing' => sub {
    foreach ( 1 .. 128 ) {
        my $v = 'x' x $_;
        my $msg = String::StringBytes::Bytes->new({ v_string => $v });
        my $p = $msg->serialize();
        my $l = length($p);

        my $q_msg = String::StringBytes::Bytes->parse($p);
        my $q = $q_msg->v_string;
        my $m = length($q);

        is($q, $v, "v_string length $_: $l bytes");
    }
};

subtest 'bytes member packing' => sub {
    foreach ( 1 .. 128 ) {
        my $v = 'x' x $_;
        my $msg = String::StringBytes::Bytes->new({ v_bytes => $v });
        my $p = $msg->serialize();
        my $l = length($p);

        my $q_msg = String::StringBytes::Bytes->parse($p);
        my $q = $q_msg->v_bytes;

        is($q, $v, "v_bytes length $_: $l bytes");
    }
};

subtest 'embedded NULLs' => sub {
    my $v = "x\0\0\0\0\0\0\0\0y";
    my $msg = String::StringBytes::Bytes->new({ v_string => $v, v_bytes => $v });
    my $p = $msg->serialize();

    my $u = String::StringBytes::Bytes->parse($p);

    is($u->v_bytes, $v, 'v_bytes with NULLs ok');
    is($u->v_string, $v, 'v_string with NULLs ok');
};

subtest 'large members' => sub {
    my $s = 'A' x (32 * 1024 * 1024);

    my $start = [gettimeofday];
    my $m = String::StringBytes::Bytes->new({ v_string => $s });
    my $elapsed = tv_interval($start);
    ok($m, "Constructed 32 MB string message in $elapsed seconds");

    $start = [gettimeofday];
    my $n = String::StringBytes::Bytes->new({ v_bytes => $s });
    $elapsed = tv_interval($start);
    ok($n, "Constructed 32 MB bytes message in $elapsed seconds");

    $start = [gettimeofday];
    my $a = $m->serialize();
    $elapsed = tv_interval($start);
    my $al = length($a);
    ok($al > 0, "Serialized 32 MB string message in $elapsed seconds ($al bytes)");

    $start = [gettimeofday];
    my $b = $n->serialize();
    $elapsed = tv_interval($start);
    my $bl = length($b);
    ok($bl > 0, "Serialized 32 MB bytes message in $elapsed seconds ($bl bytes)");

    # Verify roundtrip for large message
    $start = [gettimeofday];
    my $m2 = String::StringBytes::Bytes->parse($a);
    $elapsed = tv_interval($start);
    is(length($m2->v_string), length($s), "Parsed 32 MB string in $elapsed seconds");
};

done_testing();
