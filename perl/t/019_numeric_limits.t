use strict;
use warnings;
use Test::More;
use Math::BigInt;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest '64-bit integer limits and BigInt support' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    # Test int64
    my $val_i64 = "9223372036854775807"; # MAX_INT64
    $msg->set('optional_int64', Math::BigInt->new($val_i64));
    my $got_i64 = $msg->get('optional_int64');

    if (ref($got_i64)) {
        isa_ok($got_i64, 'Math::BigInt');
        is($got_i64->bstr(), $val_i64, 'Max int64 preserved via BigInt');
    } else {
        is($got_i64, $val_i64, 'Max int64 preserved via native 64-bit IV');
    }

    # Test negative int64
    my $val_neg_i64 = "-9223372036854775808"; # MIN_INT64
    $msg->set('optional_int64', Math::BigInt->new($val_neg_i64));
    $got_i64 = $msg->get('optional_int64');
    is($got_i64 . "", $val_neg_i64, 'Min int64 preserved');

    # Test uint64
    my $val_u64 = "18446744073709551615"; # MAX_UINT64
    $msg->set('optional_uint64', Math::BigInt->new($val_u64));
    my $got_u64 = $msg->get('optional_uint64');
    is($got_u64 . "", $val_u64, 'Max uint64 preserved');

    # Test small values (should return native IV/UV)
    $msg->set('optional_int64', 12345);
    my $small_i64 = $msg->get('optional_int64');
    ok(!ref($small_i64), 'Small int64 returned as native IV');
    is($small_i64, 12345, 'Small int64 value correct');
};

subtest '64-bit string input' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();

    $msg->set('optional_int64', "1234567890123456789");
    is($msg->get('optional_int64') . "", "1234567890123456789", 'Int64 from string');

    $msg->set('optional_uint64', "12345678901234567890");
    is($msg->get('optional_uint64') . "", "12345678901234567890", 'Uint64 from string');
};

done_testing();
