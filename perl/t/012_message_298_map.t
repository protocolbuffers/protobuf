use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'scalar map field accessors' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();

    ok($msg->can('map_int32_int32'), 'Generated getter for map_int32_int32');
    my $map = $msg->map_int32_int32;
    ok($map, 'Got map field object');
    isa_ok($map, 'HASH', 'It is a hash reference (tied)');
    is(scalar(keys %$map), 0, 'Initial size is 0');

    # Store elements
    $map->{10} = 100;
    $map->{20} = 200;
    is(scalar(keys %$map), 2, 'Size after storing is 2');
    is($map->{10}, 100, 'Key 10 has correct value');
    is($map->{20}, 200, 'Key 20 has correct value');

    # Exists
    ok(exists $map->{10}, 'exists works');
    ok(!exists $map->{30}, 'exists works for non-existent');

    # Delete
    my $deleted = delete $map->{10};
    is($deleted, 100, 'delete returns correct value');
    is(scalar(keys %$map), 1, 'Size after delete is 1');
    ok(!exists $map->{10}, 'Key 10 is gone');

    # Clear
    %$map = ();
    is(scalar(keys %$map), 0, 'Size after clear is 0');

    subtest 'as_hash' => sub {
        $map->{10} = 100;
        my $h = $map->as_hash();
        is(ref($h), 'HASH', 'as_hash returns a hash ref');
        is($h->{10}, 100, 'Data preserved in as_hash');
    };
};

subtest 'string map field accessors' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    my $map = $msg->map_string_string;

    $map->{foo} = "bar";
    $map->{baz} = "qux";

    is($map->{foo}, "bar", 'String key/value correct');

    # Iteration
    my %got;
    while (my ($k, $v) = each %$map) {
        $got{$k} = $v;
    }
    is_deeply(\%got, { foo => "bar", baz => "qux" }, 'Iteration works');
};

subtest 'message map field accessors' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    my $map = $msg->map_string_nested_message;

    my $sub1 = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage->new();
    $sub1->set_a(123);

    $map->{m1} = $sub1;

    my $got = $map->{m1};
    ok($got, 'Got message from map');
    isa_ok($got, 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage');
    is($got->a, 123, 'Message data correct');

    # Update through retrieved object
    $got->set_a(456);
    is($map->{m1}->a, 456, 'Update reflected in map');
};

TODO: {
    local $TODO = 'Implement O(1) Bulk Map Projection';
    ok(0, 'Projecting entire upb_Map to Perl hash avoids redundant FETCH overhead');
}

TODO: {
    local $TODO = 'Implement Shared-Arena Key Deduplication';
    ok(0, 'String keys are shared across maps in the same arena to save memory');
}

TODO: {
    local $TODO = 'Provide Real-Time Map Collision Analysis';
    ok(0, 'Map collision statistics are reportable via XS interface');
}

TODO: {
    local $TODO = 'Implement O(1) Map-to-Hash Projection';
    ok(0, 'Zero-copy access to entire maps from Perl verified');
}

TODO: {
    local $TODO = 'Implement SSE4.2-Accelerated Map Key Validation';
    ok(0, 'Hardware-accelerated key validation for bulk ingestion verified');
}

TODO: {
    local $TODO = 'Implement Self-Healing Map Structure Consistency';
    ok(0, 'Internal auditor detects and reports corruption in reified map objects');
}

done_testing();
