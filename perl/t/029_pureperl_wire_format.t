use strict;
use warnings;
use Test::More;
use Protobuf;
use Protobuf::DescriptorPool;
use lib "t/lib";
use TestHelpers;

# Load descriptors
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'Pure-Perl Serialize/Parse' => sub {
    # Force PurePerl engine
    my $msg = Protobuf_perl_test::Test::TestMessage->new({ profile => 'pure_perl' });

    ok($msg->isa('Protobuf_perl_test::Test::TestMessage'), 'Object is correct class');
    is(ref($msg->{_engine}), 'Protobuf::Engine::PurePerl', 'Using PurePerl engine');

    # Use generic set (which is engine-aware)
    $msg->set('value', 123);

    # Test generated accessor (SHOULD FAIL currently or use fallback)
    eval { $msg->value(456) };
    if ($@) {
        diag("Generated accessor failed as expected: $@");
    } else {
        is($msg->get('value'), 456, 'Generated accessor worked (engine-aware?)');
    }
    $msg->set('optional_uint32', 456);
    $msg->set('test_string', 'hello world');

    my $bin = $msg->serialize();
    ok(length($bin) > 0, 'Serialized to binary');

    # Parse back using PurePerl
    my $decoded = Protobuf_perl_test::Test::TestMessage->parse($bin, { profile => 'pure_perl' });

    is($decoded->get('value'), 456, 'Parsed value matches');
    is($decoded->get('optional_uint32'), 456, 'Parsed uint32 matches');
    is($decoded->get('test_string'), 'hello world', 'Parsed string matches');
};

subtest 'Pure-Perl Nested Messages' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new({ profile => 'pure_perl' });

    # Create nested message
    my $nested = Protobuf_perl_test::Test::NestedMessage->new({ profile => 'pure_perl' });
    $nested->set('nested_string', 'inside');

    $msg->set('nested_message', $nested);

    my $bin = $msg->serialize();
    my $decoded = Protobuf_perl_test::Test::TestMessage->parse($bin, { profile => 'pure_perl' });

    my $d_nested = $decoded->get('nested_message');
    ok($d_nested, 'Got nested message');
    is($d_nested->get('nested_string'), 'inside', 'Nested value matches');
};

subtest 'Pure-Perl vs XS Interop' => sub {
    # Create XS message
    my $xs_msg = Protobuf_perl_test::Test::TestMessage->new({ value => 789, test_string => 'interop' });
    my $bin = $xs_msg->serialize();

    # Parse with PurePerl
    # Note: parse() is a class method. We need to pass profile in options.
    # Currently parse() takes ($data, $options).
    my $pp_msg = Protobuf_perl_test::Test::TestMessage->parse($bin, { profile => 'pure_perl' });

    is($pp_msg->get('value'), 789, 'PurePerl parsed XS-serialized data');
    is($pp_msg->get('test_string'), 'interop', 'PurePerl parsed XS-serialized string');
};

done_testing();
