use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Time::Piece;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/wkt_descriptor.bin');

subtest 'timestamp operations' => sub {
    my $ts = Google::Protobuf::Timestamp::Timestamp->new();
    my $now = gmtime();

    $ts->from_time_piece($now);
    is($ts->seconds, $now->epoch, 'Epoch matches');

    my $tp = $ts->to_time_piece();
    is($tp->epoch, $now->epoch, 'Time::Piece back-conversion matches');

    # ISO8601
    my $iso = $ts->to_iso8601();
    like($iso, qr/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$/, 'ISO8601 format correct');
};

TODO: {
    local $TODO = 'Implement Vectorized Timestamp Conversions';
    ok(0, 'High-throughput Timestamp processing uses SSE4.1/AVX2 optimizations');
}

done_testing();
