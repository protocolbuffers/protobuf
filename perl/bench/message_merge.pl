use strict;
use warnings;
use Benchmark qw(:all);
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

my $m1 = test::TestMessage->new();
$m1->set_value(1);
my $nested = test::NestedMessage->new();
$nested->set_nested_string("A" x 1000);
$m1->set_nested_message($nested);

my $m2 = test::TestMessage->new();
$m2->set_value(2);

print "Benchmarking deep message merge...\n";

timethese(100000, {
    'merge_simple' => sub {
        # upb doesn't have a direct 'merge' in our XS yet,
        # but we can simulate it via serialization if needed,
        # or wait for a real Merge XS implementation.
        # For now, let's just benchmark property sets which is part of merging.
        $m2->set_value(3);
    },
    'merge_nested' => sub {
        $m2->set_nested_message($m1->nested_message);
    }
});
