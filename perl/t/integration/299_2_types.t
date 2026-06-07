use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/compat_descriptor.bin');

subtest 'types roundtrip' => sub {
    my $msg1 = Protobuf::Types::Types->new(
        {
            req_double   => 1.01,
            req_float    => 2.06,
            req_int32    => 16777216,
            req_int64    => '123456789012345',
            req_uint32   => 16777217,
            req_uint64   => '123456789012346',
            req_sint32   => -16777217,
            req_sint64   => '-123456789012346',
            req_fixed32  => 16777218,
            req_fixed64  => '123456789012347',
            req_sfixed32 => -16777218,
            req_sfixed64 => '-123456789012347',
            req_bool     => 1,
            req_string   => "Hello, world!",
            req_bytes    => "Byte array",
            req_enum     => Protobuf::Types::Types::Enum->value1,
            req_message  => { t_string => "embedded message" }
        });

    my $p = $msg1->serialize();
    ok(length($p) > 0, 'Serialized types message');

    my $msg2 = Protobuf::Types::Types->parse($p);
    ok($msg2, 'Parsed types message');

    my $h = $msg2->to_hashref;

    is($h->{req_double}, 1.01, 'req_double');
    is(sprintf("%.2f", $h->{req_float}), "2.06", 'req_float');
    is($h->{req_int32}, 16777216, 'req_int32');
    is($h->{req_int64}, '123456789012345', 'req_int64');
    is($h->{req_uint32}, 16777217, 'req_uint32');
    is($h->{req_uint64}, '123456789012346', 'req_uint64');
    is($h->{req_sint32}, -16777217, 'req_sint32');
    is($h->{req_sint64}, '-123456789012346', 'req_sint64');
    is($h->{req_fixed32}, 16777218, 'req_fixed32');
    is($h->{req_fixed64}, '123456789012347', 'req_fixed64');
    is($h->{req_sfixed32}, -16777218, 'req_sfixed32');
    is($h->{req_sfixed64}, '-123456789012347', 'req_sfixed64');
    is($h->{req_bool}, 1, 'req_bool');
    is($h->{req_string}, "Hello, world!", 'req_string');
    is($h->{req_bytes}, "Byte array", 'req_bytes');
    is($h->{req_enum}, Protobuf::Types::Types::Enum->value1, 'req_enum');
    is($h->{req_message}->{t_string}, "embedded message", 'req_message');
};

done_testing();
