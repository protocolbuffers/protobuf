package main;
use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
# Load both test and WKT descriptors
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin', 't/data/wkt_descriptor.bin');

subtest 'any packed with other message' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(999);

    my $any = Google::Protobuf::Any::Any->new();
    $any->pack($msg);

    my $unpacked = $any->unpack();
    is($unpacked->value, 999, 'Any packs/unpacks TestMessage correctly');
};

subtest 'struct inside listvalue inside struct' => sub {
    my $data = {
        key => [
            { a => 1 },
            { b => "two" }
        ]
    };

    my $struct = Google::Protobuf::Struct::Struct->new();
    $struct->from_perl($data);

    my $out = $struct->to_perl();
    is_deeply($out, $data, 'Complex nested struct roundtrip works');

    my $serialized = $struct->serialize();
    my $parsed = Google::Protobuf::Struct::Struct->parse($serialized);
    is_deeply($parsed->to_perl, $data, 'Struct serialization roundtrip works');
};

subtest 'struct json bridge' => sub {
    my $struct = Google::Protobuf::Struct::Struct->new();
    my $json = $struct->to_json();
    ok($json, 'Got JSON from struct');
    is($json, '{}', 'Matches skeletal expectation');
};

TODO: {
    local $TODO = 'Implement Direct Struct-to-JSON Bridge (C-Layer)';
    ok(0, 'High-performance Struct serialization bypasses intermediate Perl hashes');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter WKT Registry';
    ok(0, 'WKT definitions in shared pools are consistent across interpreters');
}

TODO: {
    local $TODO = 'Verify High-Pressure Temporal Stress stability';
    ok(0, 'System handles 10,000 concurrent Timestamp/Duration conversions safely');
}

done_testing();
