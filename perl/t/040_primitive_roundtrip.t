use strict;
use warnings;
use Test::More;
use lib 't/lib';
use TestHelpers;
use Math::BigInt;

my $pool = TestHelpers->get_generated_pool();
diag("Using engine: " . ($Protobuf::HAS_XS ? 'XS' : 'PurePerl'));
my $files = TestHelpers->load_test_protos($pool, 't/data/types_descriptor.bin');
foreach my $f (@$files) {
    Protobuf::ClassGenerator->generate_for_file($f);
}

my @types = (
    { name => 'int32',    field => 'opt_int32',    vals => [-2147483648, 0, 2147483647] },
    { name => 'uint32',   field => 'opt_uint32',   vals => [0, 4294967295] },
    { name => 'sint32',   field => 'opt_sint32',   vals => [-2147483648, 0, 2147483647] },
    { name => 'fixed32',  field => 'opt_fixed32',  vals => [0, 4294967295] },
    { name => 'sfixed32', field => 'opt_sfixed32', vals => [-2147483648, 0, 2147483647] },
    { name => 'int64',    field => 'opt_int64',    vals => ["-9223372036854775808", "0", "9223372036854775807"] },
    { name => 'uint64',   field => 'opt_uint64',   vals => ["0", "18446744073709551615"] },
    { name => 'sint64',   field => 'opt_sint64',   vals => ["-9223372036854775808", "0", "9223372036854775807"] },
    { name => 'fixed64',  field => 'opt_fixed64',  vals => ["0", "18446744073709551615"] },
    { name => 'sfixed64', field => 'opt_sfixed64', vals => ["-9223372036854775808", "0", "9223372036854775807"] },
    { name => 'float',    field => 'opt_float',    vals => [-3.4e38, 0, 3.4e38] },
    { name => 'double',   field => 'opt_double',   vals => [-1.7e308, 0, 1.7e308] },
    { name => 'bool',     field => 'opt_bool',     vals => [0, 1] },
    { name => 'string',   field => 'opt_string',   vals => ['', 'hello world', "\x{1f600}"] },
    { name => 'bytes',    field => 'opt_bytes',    vals => ['', "\x00\xff\x00\xff"] },
);

foreach my $t (@types) {
    subtest "Primitive Roundtrip: $t->{name}" => sub {
        foreach my $v (@{$t->{vals}}) {
            my $msg = Protobuf::Types::Types->new();
            my $field = $t->{field};
            my $setter = "set_$field";

            my $input = $v;
            if ($t->{name} =~ /64/) {
                $input = Math::BigInt->new($v);
            }

            eval { $msg->$setter($input) };
            ok(!$@, "Set $t->{name} = $v") or diag($@);

            my $got = $msg->$field();
            if ($t->{name} =~ /64/) {
                my $got_str = ref($got) ? $got->bstr() : "$got";
                my $expected_str = Math::BigInt->new($v)->bstr();
                is($got_str, $expected_str, "Value matches original (64-bit)");
            } elsif ($t->{name} eq 'float') {
                # Allow some precision loss for float
                ok(abs($got - $v) / (abs($v) || 1) < 1e-6, "Value matches original (float)");
            } elsif ($t->{name} eq 'bool') {
                if ($v) { ok($got, "Value matches original (bool true)"); }
                else { ok(!$got, "Value matches original (bool false)"); }
            } elsif ($t->{name} eq 'string') {
                utf8::decode($got) if !utf8::is_utf8($got);
                my $expected = $v;
                utf8::decode($expected) if !utf8::is_utf8($expected);
                is($got, $expected, "Value matches original (string)");
            } else {
                is($got, $v, "Value matches original");
            }

            # Wire roundtrip
            my $data = $msg->serialize();
            my $msg2 = Protobuf::Types::Types->parse($data);
            my $got2 = $msg2->$field();

            if ($t->{name} =~ /64/) {
                my $got2_str = ref($got2) ? $got2->bstr() : "$got2";
                my $expected_str = Math::BigInt->new($v)->bstr();
                is($got2_str, $expected_str, "Value matches after wire roundtrip (64-bit)");
            } elsif ($t->{name} eq 'float') {
                ok(abs($got2 - $v) / (abs($v) || 1) < 1e-6, "Value matches after wire roundtrip (float)");
            } elsif ($t->{name} eq 'bool') {
                if ($v) { ok($got2, "Value matches after wire roundtrip (bool true)"); }
                else { ok(!$got2, "Value matches after wire roundtrip (bool false)"); }
            } elsif ($t->{name} eq 'string') {
                utf8::decode($got2) if !utf8::is_utf8($got2);
                my $expected = $v;
                utf8::decode($expected) if !utf8::is_utf8($expected);
                is($got2, $expected, "Value matches after wire roundtrip (string)");
            } else {
                is($got2, $v, "Value matches after wire roundtrip");
            }
        }
    };
}

done_testing();
