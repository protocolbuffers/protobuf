use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
# Load both test and WKT descriptors
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin', 't/data/wkt_descriptor.bin');

subtest 'any packing and unpacking' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(12345);

    my $any = Google::Protobuf::Any::Any->new();
    $any->pack($msg);

    is($any->type_url, "type.googleapis.com/protobuf_perl_test.TestMessage", 'Any type_url correct');
    ok(length($any->value) > 0, 'Any value contains data');

    # Unpack
    my $unpacked = $any->unpack();
    isa_ok($unpacked, 'Protobuf_perl_test::Test::TestMessage');
    is($unpacked->value, 12345, 'Unpacked data matches original');
};

TODO: {
    local $TODO = 'Implement Direct Any Unpacking (C-Layer)';
    ok(0, 'Unpacking Any directly to C structures avoids intermediate Perl objects');
}

done_testing();
