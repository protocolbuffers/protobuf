use strict;
use warnings;
use Test::More;
use Protobuf;
use Protobuf::Message;
use Protobuf::UnknownFieldSet;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'Selective Tag Scrubbing' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    # Manually add some unknown fields with specific tags
    # Tag 100, wire type 0 (varint), value 1: 0xA0 0x06 0x01 -> wait, tag is (field_num << 3) | wire_type
    # Tag 100: (100 << 3) | 0 = 800. 800 in varint: 0xA0 0x06
    my $tag100_data = pack("C*", 0xA0, 0x06, 0x01);
    # Tag 101: (101 << 3) | 0 = 808. 808 in varint: 0xA8, 0x06
    my $tag101_data = pack("C*", 0xA8, 0x06, 0x02);

    my $set = $msg->unknown_fields;
    $set->add($tag100_data);
    $set->add($tag101_data);
    $set->add($tag100_data); # Add tag 100 again

    is(length($set->get_data()), 9, "Initial unknown data length is 9 (3 entries of 3 bytes)");

    # Delete tag 100
    $set->delete_tag(100);

    my $rem = $set->get_data();
    is(length($rem), 3, "Length after deleting tag 100 is 3");
    is(unpack("H*", $rem), unpack("H*", $tag101_data), "Only tag 101 remains");

    # Delete tag 101
    $set->delete_tag(101);
    is(length($set->get_data()), 0, "No data remains after deleting tag 101");
};

done_testing();
