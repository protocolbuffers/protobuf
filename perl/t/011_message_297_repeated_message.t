use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'repeated message field accessors' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    ok($msg->can('repeated_message'), 'Generated getter for repeated_message');
    my $arr = $msg->repeated_message;
    ok($arr, 'Got repeated field object');
    isa_ok($arr, 'ARRAY', 'It is an array reference (tied)');
    is(scalar(@$arr), 0, 'Initial size is 0');

    # Create sub-messages
    my $sub1 = Protobuf_perl_test::Test::NestedMessage->new();
    $sub1->set_nested_string("val1");

    my $sub2 = Protobuf_perl_test::Test::NestedMessage->new();
    $sub2->set_nested_string("val2");

    # Push elements
    push @$arr, $sub1, $sub2;
    is(scalar(@$arr), 2, 'Size after push is 2');

    my $got1 = $arr->[0];
    isa_ok($got1, 'Protobuf_perl_test::Test::NestedMessage');
    is($got1->nested_string, "val1", 'Index 0 has correct data');

    my $got2 = $arr->[1];
    is($got2->nested_string, "val2", 'Index 1 has correct data');

    # Verify shared arena (indirectly)
    $got1->set_nested_string("val3");
    is($arr->[0]->nested_string, "val3", 'Update through retrieved object reflected in array');
};

subtest 'assignment by index with sub-messages' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    my $arr = $msg->repeated_message;

    my $sub = Protobuf_perl_test::Test::NestedMessage->new();
    $sub->set_nested_string("orig");

    push @$arr, $sub;
    is($arr->[0]->nested_string, "orig", 'Initial value');

    my $sub2 = Protobuf_perl_test::Test::NestedMessage->new();
    $sub2->set_nested_string("new");

    $arr->[0] = $sub2;
    is($arr->[0]->nested_string, "new", 'Value updated after assignment');
};

subtest 'pop and shift with sub-messages' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    my $arr = $msg->repeated_message;

    foreach my $val ("a", "b") {
        my $sub = Protobuf_perl_test::Test::NestedMessage->new();
        $sub->set_nested_string($val);
        push @$arr, $sub;
    }

    is(scalar(@$arr), 2, 'Size is 2');

    my $popped = pop @$arr;
    is($popped->nested_string, "b", 'Popped correct message');
    is(scalar(@$arr), 1, 'Size is 1');

    my $shifted = shift @$arr;
    is($shifted->nested_string, "a", 'Shifted correct message');
    is(scalar(@$arr), 0, 'Size is 0');
};

done_testing();
