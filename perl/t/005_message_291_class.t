use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;
use Protobuf::Message;

my $pool = TestHelpers->get_empty_pool();

# This should trigger ClassGenerator for protobuf_perl_test.TestMessage and protobuf_perl_test.NestedMessage
my $file = TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');
ok($file, 'Added file descriptor set');

# Check if classes were generated
ok(Protobuf_perl_test::Test::TestMessage->can('new'), 'Protobuf_perl_test::Test::TestMessage class was generated and can new()');
ok(Protobuf_perl_test::Test::NestedMessage->can('new'), 'Protobuf_perl_test::Test::NestedMessage class was generated and can new()');

# Instantiate
my $msg = Protobuf_perl_test::Test::TestMessage->new();
ok($msg, 'Instantiated Protobuf_perl_test::Test::TestMessage');
isa_ok($msg, 'Protobuf_perl_test::Test::TestMessage');
isa_ok($msg, 'Protobuf::Message');

done_testing();
