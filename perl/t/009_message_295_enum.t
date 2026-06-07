use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'enum field accessors' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    # Test Enum: protobuf_perl_test.TestEnum { FOO = 0, BAR = 1, BAZ = 2 }
    ok($msg->can('enum_field'), 'Generated getter for enum_field');
    is($msg->enum_field, 0, 'Default value for enum is 0 (FOO)');

    ok($msg->can('set_enum_field'), 'Generated setter for enum_field');

    # Set by integer
    $msg->set_enum_field(1);
    is($msg->enum_field, 1, 'Set enum by integer (TEST_ENUM_FIRST)');

    # Set by name
    $msg->set_enum_field('TEST_ENUM_SECOND');
    is($msg->enum_field, 2, 'Set enum by string name (TEST_ENUM_SECOND)');

    # Invalid integer (optional validation, UPB might allow it if not strictly checked)
    # Proto3 allows open enums, Proto2 is stricter.
    # Our test_descriptor.bin is likely Proto3.

    # Invalid name
    eval { $msg->set_enum_field('INVALID_NAME') };
    ok($@, 'Setting invalid enum name throws exception');
};

done_testing();
