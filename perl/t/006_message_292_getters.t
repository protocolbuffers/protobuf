use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'scalar field getters' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    # Test setting a field through the base get/set methods
    $msg->set('value', 42);
    $msg->set('optional_uint32', 123);

    # Now verify the generated getters work
    ok($msg->can('value'), 'Generated getter for value');
    is($msg->value, 42, 'Getter returns correct value for value');

    ok($msg->can('optional_uint32'), 'Generated getter for optional_uint32');
    is($msg->optional_uint32, 123, 'Getter returns correct value for optional_uint32');
};

done_testing();
