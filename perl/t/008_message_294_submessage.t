use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'sub-message field accessors' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    ok($msg->can('nested_message'), 'Generated getter for nested_message');
    is($msg->nested_message, undef, 'Getter returns undef for unset sub-message');

    ok($msg->can('set_nested_message'), 'Generated setter for nested_message');

    my $nested = Protobuf_perl_test::Test::NestedMessage->new();
    $nested->set_nested_string("nested-val");

    $msg->set_nested_message($nested);
    ok($msg->has_nested_message, 'has_ returns true after setting sub-message');

    my $got_nested = $msg->nested_message;
    ok($got_nested, 'Got nested message');
    isa_ok($got_nested, 'Protobuf_perl_test::Test::NestedMessage');
    is($got_nested->nested_string, "nested-val", 'Nested message has correct data');
};

subtest 'shared arena' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    my $nested = Protobuf_perl_test::Test::NestedMessage->new();

    $msg->set_nested_message($nested);

    my $got_nested = $msg->nested_message;

    # We should verify that they share the same arena.
    # In our implementation, the Perl wrapper for the sub-message should hold
    # a reference to the same Arena SV as the parent.

    # We don't have a public method to get the arena SV, but we can verify
    # that changes to the sub-message are reflected in the parent's serialized form.

    $got_nested->set_nested_string("updated");

    my $data = $msg->serialize();
    my $msg2 = Protobuf_perl_test::Test::TestMessage->parse($data);
    is($msg2->nested_message->nested_string, "updated", 'Sub-message update reflected in parent');
};

subtest 'clearing sub-message' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    my $nested = Protobuf_perl_test::Test::NestedMessage->new();
    $msg->set_nested_message($nested);

    ok($msg->has_nested_message, 'Has nested message');
    $msg->clear_nested_message();
    ok(!$msg->has_nested_message, 'Cleared nested message');
    is($msg->nested_message, undef, 'Getter returns undef after clearing');
};

done_testing();
