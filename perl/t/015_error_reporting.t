use strict;
use warnings;
use Test::More;
use lib 't/lib';
use TestHelpers;
use Protobuf;

my $pool = TestHelpers->get_empty_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

subtest 'Enhanced Parse Error Reporting' => sub {
    my $class = 'Protobuf_perl_test::Test::TestMessage';

    # 1. Malformed Varint
    eval {
        $class->parse("\x08\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01");
    };
    like($@, qr/Failed to parse message: Protobuf wire format is malformed or corrupt/, 'Descriptive error for malformed varint');

    # 2. Unexpected EOF (partial tag)
    eval {
        $class->parse("\x08");
    };
    like($@, qr/Failed to parse message: Protobuf wire format is malformed or corrupt/, 'Descriptive error for unexpected EOF');

    # 3. Bad UTF-8 (if UPB validates it during decode)
    # Note: upb_Decode usually doesn't validate UTF-8 unless configured,
    # but let's check if our mapping is functional via eval.

    # 4. Missing Required Fields (proto2)
    # TestMessage in test.proto is proto3-like or doesn't have required fields.
    # Let's check if we have a proto2 message with required fields.
};

subtest 'Detailed Conflict Metadata' => sub {
    my $bin_data = path('t/data/test_descriptor.bin')->slurp_raw;

    # Try to add the same file again to the same pool
    eval {
        $pool->add_serialized_file_descriptor_set($bin_data);
    };
    like($@, qr/already defined in this DescriptorPool/, 'Conflict error contains source pool info');
};

done_testing();

use Path::Tiny;
