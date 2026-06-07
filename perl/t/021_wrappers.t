use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/wkt_descriptor.bin');

subtest 'wrappers operations' => sub {
    my $d = Google::Protobuf::Wrappers::DoubleValue->new();
    $d->set_value(3.14);
    is($d->value, 3.14, 'DoubleValue works');

    my $f = Google::Protobuf::Wrappers::FloatValue->new();
    $f->set_value(2.71);
    # Float precision comparison
    ok(abs($f->value - 2.71) < 0.001, 'FloatValue works');

    my $i64 = Google::Protobuf::Wrappers::Int64Value->new();
    $i64->set_value(9000000000);
    is($i64->value, 9000000000, 'Int64Value works');

    my $ui64 = Google::Protobuf::Wrappers::UInt64Value->new();
    $ui64->set_value(9000000000);
    is($ui64->value, 9000000000, 'UInt64Value works');

    my $i32 = Google::Protobuf::Wrappers::Int32Value->new();
    $i32->set_value(-12345);
    is($i32->value, -12345, 'Int32Value works');

    my $ui32 = Google::Protobuf::Wrappers::UInt32Value->new();
    $ui32->set_value(12345);
    is($ui32->value, 12345, 'UInt32Value works');

    my $b = Google::Protobuf::Wrappers::BoolValue->new();
    $b->set_value(1);
    ok($b->value, 'BoolValue true works');
    $b->set_value(0);
    ok(!$b->value, 'BoolValue false works');

    my $s = Google::Protobuf::Wrappers::StringValue->new();
    $s->set_value("hello");
    is($s->value, "hello", 'StringValue works');

    my $by = Google::Protobuf::Wrappers::BytesValue->new();
    $by->set_value("bytes\x00data");
    is($by->value, "bytes\x00data", 'BytesValue works');
};

done_testing();
