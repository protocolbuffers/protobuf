use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'scalar field mutators (setters)' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    ok($msg->can('set_value'), 'Generated setter for value');
    $msg->set_value(42);
    is($msg->value, 42, 'Setter correctly sets value');

    ok($msg->can('set_optional_uint32'), 'Generated setter for optional_uint32');
    $msg->set_optional_uint32(123);
    is($msg->optional_uint32, 123, 'Setter correctly sets optional_uint32');
};

subtest 'scalar field has_ methods' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    ok($msg->can('has_optional_uint32'), 'Generated has_ method for optional_uint32');
    ok(!$msg->has_optional_uint32, 'has_ returns false initially');

    $msg->set_optional_uint32(456);
    ok($msg->has_optional_uint32, 'has_ returns true after setting');
};

subtest 'scalar field clear_ methods' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    $msg->set_optional_uint32(789);
    ok($msg->has_optional_uint32, 'Field is set');

    ok($msg->can('clear_optional_uint32'), 'Generated clear_ method for optional_uint32');
    $msg->clear_optional_uint32();
    ok(!$msg->has_optional_uint32, 'has_ returns false after clearing');
    is($msg->optional_uint32, 0, 'Getter returns default after clearing');
};

done_testing();
