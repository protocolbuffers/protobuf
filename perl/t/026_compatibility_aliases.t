use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Protobuf;

my $pool = TestHelpers->get_empty_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'G::PB Compatibility Aliases' => sub {
    my $class = "Protobuf_perl_test::Test::TestMessage";
    my $msg = $class->new(value => 123, name => "alias test");

    # 1. encode / decode
    my $wire = $msg->encode();
    ok($wire, "encode() works as alias for serialize()");

    my $msg2 = $class->decode($wire);
    is($msg2->name, "alias test", "decode() works as alias for parse()");

    # 2. toJSON / fromJSON
    SKIP: {
        skip "JSON is not supported in PurePerl mode", 2 unless $Protobuf::HAS_XS;
        my $json = $msg->toJSON();
        like($json, qr/"name":\s*"alias test"/, "toJSON() works as alias for to_json()");

        my $msg3 = $class->fromJSON($json);
        is($msg3->name, "alias test", "fromJSON() works as alias for from_json()");
    }
};

done_testing();
