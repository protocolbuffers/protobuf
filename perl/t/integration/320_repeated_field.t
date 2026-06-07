package main;
use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Protobuf::Internal::Repeated;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'repeated field cross-message interaction' => sub {
    my $msg1 = Protobuf_perl_test::Test::TestMessage->new();
    push @{$msg1->repeated_int}, 10, 20, 30;

    my $msg2 = Protobuf_perl_test::Test::TestMessage->new();
    # This should copy elements
    $msg2->set('repeated_int', $msg1->repeated_int);

    is_deeply($msg2->repeated_int, [10, 20, 30], 'Repeated field copied to another message');

    # Verify they are independent
    push @{$msg1->repeated_int}, 40;
    is(scalar(@{$msg2->repeated_int}), 3, 'Target message independent of source after copy');
};

subtest 'repeated message cross-message interaction' => sub {
    my $msg1 = Protobuf_perl_test::Test::TestMessage->new();
    my $sub = Protobuf_perl_test::Test::NestedMessage->new();
    $sub->set_nested_string("orig");
    push @{$msg1->repeated_message}, $sub;

    my $msg2 = Protobuf_perl_test::Test::TestMessage->new();
    $msg2->set('repeated_message', $msg1->repeated_message);

    is($msg2->repeated_message->[0]->nested_string, "orig", 'Repeated messages copied');

    # Verify independent
    $msg1->repeated_message->[0]->set_nested_string("changed");
    is($msg2->repeated_message->[0]->nested_string, "orig", 'Messages in target are independent (copied)');
};

subtest 'array slicing' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    my $arr = $msg->repeated_int;
    push @$arr, 1, 2, 3;

    # We need a blessed object for slice() method
    my $wrapper = bless $arr, 'Protobuf::Internal::Repeated::Public';
    my $slice = $wrapper->slice(0, 1);
    ok($slice, 'Got slice');
    is(ref($slice), 'ARRAY', 'Slice is an array ref');
};

TODO: {
    local $TODO = 'Implement Direct Array-to-Array Deep Copy';
    ok(0, 'Assigning one repeated field to another uses C-level cloning');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter Container Migration Stress';
    ok(0, 'Tied arrays maintain consistent state when migrated across interpreters');
}

subtest 'to_perl' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    push @{$msg->repeated_int}, 1, 2, 3;

    my $wrapper = $msg->repeated_int;
    my $perl_data = $wrapper->to_perl();

    is_deeply($perl_data, [1, 2, 3], 'to_perl on repeated primitives returns raw array ref');

    my $msg_m = Protobuf_perl_test::Test::TestMessage->new();
    my $sub = Protobuf_perl_test::Test::NestedMessage->new();
    $sub->set_nested_string('test');
    push @{$msg_m->repeated_message}, $sub;

    my $wrapper_m = $msg_m->repeated_message;
    my $perl_data_m = $wrapper_m->to_perl();

    is_deeply($perl_data_m, [{ nested_string => 'test' }], 'to_perl on repeated messages calls to_perl on elements');
};

done_testing();
