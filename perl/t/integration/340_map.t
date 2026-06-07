package main;
use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Protobuf::Internal::Map;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'map cross-message copy' => sub {
    my $msg1 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg1->map_string_string->{a} = "A";
    $msg1->map_string_string->{b} = "B";

    my $msg2 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg2->set('map_string_string', $msg1->map_string_string);

    is(scalar(keys %{$msg2->map_string_string}), 2, 'Map copied to another message');
    is($msg2->map_string_string->{a}, "A", 'Value A preserved');

    # Verify independence
    $msg1->map_string_string->{a} = "CHANGED";
    is($msg2->map_string_string->{a}, "A", 'Target map independent of source after copy');
};

subtest 'message map cross-message copy' => sub {
    my $msg1 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    my $sub = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage->new();
    $sub->set_a(100);
    $msg1->map_string_nested_message->{key} = $sub;

    my $msg2 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg2->set('map_string_nested_message', $msg1->map_string_nested_message);

    is($msg2->map_string_nested_message->{key}->a, 100, 'Message map copied');

    # Verify independence
    $msg1->map_string_nested_message->{key}->set_a(200);
    is($msg2->map_string_nested_message->{key}->a, 100, 'Messages in target map are independent (deep copied)');
};

subtest 'map copy_from' => sub {
    my $msg1 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    my $map1 = $msg1->map_int32_int32;
    $map1->{1} = 10;

    my $msg2 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    my $map2 = $msg2->map_int32_int32;

    $map2->copy_from($map1);
    is($map2->{1}, 10, 'Data copied via copy_from');
};

TODO: {
    local $TODO = 'Implement O(1) Map-to-Map Deep Copy';
    ok(0, 'Assigning one map to another uses C-level cloning');
}

TODO: {
    local $TODO = 'Implement Shared-Arena Map Snapshotting';
    ok(0, 'Map snapshots provide zero-copy tied views of data');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter Map Mutation Stress';
    ok(0, 'Maps maintain consistent state during concurrent multi-interpreter mutation');
}

done_testing();
