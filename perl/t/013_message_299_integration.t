use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'basic serialization and parsing' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(12345);
    $msg->set_test_string("hello integration");

    my $data = $msg->serialize();
    ok(length($data) > 0, 'Serialized data is not empty');

    my $msg2 = Protobuf_perl_test::Test::TestMessage->parse($data);
    ok($msg2, 'Parsed message');
    isa_ok($msg2, 'Protobuf_perl_test::Test::TestMessage');
    is($msg2->value, 12345, 'Parsed value matches');
    is($msg2->test_string, "hello integration", 'Parsed string matches');
};

subtest 'complex message roundtrip' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg->set_optional_int32(42);

    my $sub = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage->new();
    $sub->set_a(123);
    $msg->set_optional_nested_message($sub);

    push @{$msg->repeated_int32}, 100, 200;
    $msg->map_string_string->{key1} = "val1";

    my $data = $msg->serialize();
    my $msg2 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->parse($data);

    is($msg2->optional_int32, 42, 'Top level value');
    is($msg2->optional_nested_message->a, 123, 'Nested message value');
    is_deeply($msg2->repeated_int32, [100, 200], 'Repeated field value');
    is($msg2->map_string_string->{key1}, "val1", 'Map field value');
};

subtest 'unknown fields preservation' => sub {
    # Create a message with a field tag 999 (not in the proto)
    # We can do this by appending raw wire data.
    # Tag 999, type varint (0), value 123
    # 999 << 3 | 0 = 7992 (0x1F38)
    # Varint 7992 = 0xB8 0x3E
    # Value 123 = 0x7B
    my $unknown_raw = pack("C*", 0xB8, 0x3E, 0x7B);

    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->set_value(10);
    my $base_data = $msg->serialize();

    my $combined_data = $base_data . $unknown_raw;

    my $msg2 = Protobuf_perl_test::Test::TestMessage->parse($combined_data);
    is($msg2->value, 10, 'Parsed base field');

    # Reserialize and check if unknown field is still there
    my $reserialized = $msg2->serialize();
    ok($reserialized =~ /\Q$unknown_raw\E/, 'Unknown field preserved in reserialization');
};

done_testing();
