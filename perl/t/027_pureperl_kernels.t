use strict;
use warnings;
use Test::More;
use Protobuf::Engine::PurePerl;

my $engine = Protobuf::Engine::PurePerl->new();

subtest 'Varint encoding/decoding' => sub {
    my @cases = (
        0,
        1,
        127,
        128,
        300,
        16383,
        16384,
        -1,
        -12345,
        0x7FFFFFFF,
        0xFFFFFFFF,
        0x7FFFFFFFFFFFFFFF,
    );

    foreach my $val (@cases) {
        my $encoded = Protobuf::Engine::PurePerl::_encode_varint($val);
        my $pos = 0;
        my $decoded = Protobuf::Engine::PurePerl::_decode_varint(\$encoded, \$pos);

        # For negative numbers, we need to handle the bit representation
        if ($val < 0) {
            # In Perl, bitwise results are based on native IV.
            # PB varints for negative numbers are 10 bytes and represent the 64-bit unsigned bit pattern.
            is($pos, 10, "Negative number $val encoded as 10 bytes");
            # We check if the bit patterns match. On 64-bit Perl, -1 is 0xFFFFFFFFFFFFFFFF.
            is(sprintf("%x", $decoded), sprintf("%x", $val), "Bit pattern matches for $val");
        } else {
            is($decoded, $val, "Roundtrip for $val");
        }
    }
};

subtest 'ZigZag encoding/decoding' => sub {
    my @cases = (
        [0, 0],
        [-1, 1],
        [1, 2],
        [-2, 3],
        [2147483647, 4294967294],
        [-2147483648, 4294967295],
    );

    foreach my $case (@cases) {
        my ($val, $expected) = @$case;
        my $encoded = Protobuf::Engine::PurePerl::_encode_zigzag32($val);
        is($encoded, $expected, "ZigZag32($val) == $expected");
        my $decoded = Protobuf::Engine::PurePerl::_decode_zigzag32($encoded);
        is($decoded, $val, "ZigZag32 roundtrip for $val");
    }

    # 64-bit ZigZag
    my $v64 = "-1234567890123456789";
    my $enc64 = Protobuf::Engine::PurePerl::_encode_zigzag64($v64);
    my $dec64 = Protobuf::Engine::PurePerl::_decode_zigzag64($enc64);
    is($dec64, $v64, "ZigZag64 roundtrip for $v64");
};

done_testing();
