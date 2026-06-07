package main;
use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;
use Protobuf::Message;

# Ensure Mojolicious is loaded if available for Mojo tests
my $has_mojo = eval { require Mojo::IOLoop; 1 };

subtest 'message and descriptor pool integration' => sub {
    my $pool = TestHelpers->get_empty_pool();

    TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

    subtest 'class identity and descriptors' => sub {
        my $msg = Protobuf_perl_test::Test::TestMessage->new();
        ok($msg, 'Created TestMessage');

        my $mdef = $msg->descriptor();
        ok($mdef, 'Got descriptor from message instance');
        is($mdef->full_name, 'protobuf_perl_test.TestMessage', 'Descriptor has correct full name');

        my $mdef_from_pool = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
        is("$mdef", "$mdef_from_pool", 'Message descriptor matches descriptor from pool (identity)');
    };

    subtest 'nested message integration' => sub {
        my $msg = Protobuf_perl_test::Test::TestMessage->new();

        my $sub = Protobuf_perl_test::Test::NestedMessage->new();
        $sub->set_nested_string("integration-test");

        $msg->set_nested_message($sub);

        my $got_sub = $msg->nested_message;
        ok($got_sub, 'Got nested message');
        is($got_sub->nested_string, "integration-test", 'Nested string matches');

        # Verify sub-message descriptor
        my $sub_mdef = $got_sub->descriptor();
        is($sub_mdef->full_name, 'protobuf_perl_test.NestedMessage', 'Sub-message has correct descriptor');
    };

    subtest 'arena sharing and persistence' => sub {
        my $msg = Protobuf_perl_test::Test::TestMessage->new();
        my $arr = $msg->repeated_int;
        push @$arr, 1, 2, 3;

        # In our implementation, $msg owns the arena.
        # $arr is a tied array that should hold a reference to the arena SV.
        # If we undef $msg, $arr should still work.
        undef $msg;

        is(scalar(@$arr), 3, 'Repeated array still accessible after parent message is destroyed');
        is($arr->[0], 1, 'Data preserved in repeated array');
    };

    subtest 'dependency graph' => sub {
        my $msg = Protobuf_perl_test::Test::TestMessage->new();
        my $graph = $msg->dependency_graph();
        ok($graph, 'Got dependency graph');
        is($graph->{root}, 'Protobuf_perl_test::Test::TestMessage', 'Root matches class');
    };
};

TODO: {
    local $TODO = 'Implement Direct-to-Disk Zero-Copy Serialization';
    ok(0, 'Streaming serialization to file descriptor avoids intermediate Perl strings');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter Object Identity Stability';
    ok(0, 'Messages maintain ObjCache identity when migrated across interpreters');
}

done_testing();
