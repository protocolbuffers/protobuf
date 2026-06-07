package main;
use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'text format encoding integration' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(12345);
    $msg->set_name("integration test");

    my $text = $msg->to_text();
    ok($text, 'Encoded text');
    like($text, qr/value: 12345/, 'Contains value');

    # Test text formatting doesn't corrupt message or arena
    my $msg2 = Protobuf_perl_test::Test::TestMessage->new();
    $msg2->set_value(999);
    $msg2->to_text();

    is($msg->value, 12345, 'Message data intact after text encoding');
};

subtest 'text to wire bridge' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(10);
    my $wire = $msg->to_wire();
    ok($wire, 'Got wire from message (via bridge)');
};

TODO: {
    local $TODO = 'Implement Direct TextFormat-to-Wire Conversion';
    ok(0, 'Parsing TextFormat directly to wire format bypasses intermediate Perl objects');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter TextFormat Redaction consistency';
    ok(0, 'Redaction policies are stable and consistent across interpreters');
}

TODO: {
    local $TODO = 'Verify High-Pressure Text Parsing stability';
    ok(0, 'System handles 10,000 concurrent TextFormat parses safely');
}

done_testing();
