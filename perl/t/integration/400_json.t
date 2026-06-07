package main;
use strict;
use warnings;
use Test::More;
use Protobuf;
plan skip_all => 'JSON is not supported in PurePerl mode' unless $Protobuf::HAS_XS;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'json integration' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(123);
    $msg->set_name("integration");

    my $json = $msg->to_json();

    my $msg2 = Protobuf_perl_test::Test::TestMessage->from_json($json);

    is($msg2->value, 123, 'Value matches after json roundtrip');
    is($msg2->name, 'integration', 'Name matches after json roundtrip');
};

subtest 'json compact bridge' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(10);
    my $json = $msg->to_json_compact();
    ok($json, 'Got compact JSON from message');
    unlike($json, qr/\s/, 'Compact JSON has no whitespace');
};

TODO: {
    local $TODO = 'Implement Direct JSON-to-Wire Bulk Processor';
    ok(0, 'Bulk processing of JSON streams to wire format bypasses Perl objects');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter JSON Shared Buffer consistency';
    ok(0, 'JSON parsing/generation is stable when sharing buffers across interpreters');
}

TODO: {
    local $TODO = 'Verify JSON Stress under Memory Limits';
    ok(0, 'System handles JSON parser OOM conditions gracefully under pressure');
}

done_testing();
