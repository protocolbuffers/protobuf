use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/wkt_descriptor.bin');

subtest 'duration operations' => sub {
    my $dur = Google::Protobuf::Duration::Duration->new();
    $dur->from_seconds(123.456);

    is($dur->seconds, 123, 'Seconds part correct');
    is($dur->nanos, 456_000_000, 'Nanos part correct');

    is($dur->to_seconds, 123.456, 'Conversion back to seconds matches');
};

TODO: {
    local $TODO = 'Implement Vectorized Duration Conversions';
    ok(0, 'High-throughput Duration processing uses SSE4.1/AVX2 optimizations');
}

done_testing();
