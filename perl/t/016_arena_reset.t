use strict;
use warnings;
use Test::More;
use lib 't/lib';
use TestHelpers;
use Protobuf;

# Check for Test::LeakTrace
BEGIN {
    eval "use Test::LeakTrace";
    if ($@) {
        plan skip_all => "Test::LeakTrace required for memory leak testing";
    }
}

my $pool = TestHelpers->get_empty_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');
my $class = 'Protobuf_perl_test::Test::TestMessage';

subtest 'Arena Cleanup on Parse Failure' => sub {
    no_leaks_ok {
        eval {
            # Malformed data (bad varint)
            $class->parse("\x08\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01");
        };
    } 'No leaks when parsing malformed data';
};

subtest 'Arena Cleanup on JSON Parse Failure' => sub {
    no_leaks_ok {
        eval {
            # Malformed JSON
            $class->from_json('{"value": "unclosed');
        };
    } 'No leaks when parsing malformed JSON';
};

done_testing();
