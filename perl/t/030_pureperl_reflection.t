use strict;
use warnings;
use Test::More;
use Path::Tiny;
use lib "t/lib";

# We need to load Protobuf before setting HAS_XS, but we want to simulate XS being unavailable.
# Actually, we can just set the env var or manipulate the variable after loading.
use Protobuf;
$Protobuf::HAS_XS = 0;

use Protobuf::DescriptorPool;

subtest 'Pure-Perl Pool Creation and Loading' => sub {
    my $pool = Protobuf::DescriptorPool->new();
    ok($pool, 'Created new pool');
    ok(!$pool->{_pool_ptr}, 'XS pool pointer is undef');
    ok($pool->_pp_pool, 'PurePerl pool object exists');

    my $bin_data = path('t/data/test_descriptor.bin')->slurp_raw;
    my $files = $pool->add_serialized_file_descriptor_set($bin_data);

    ok(ref($files) eq 'ARRAY', 'Added descriptor set');
    ok(scalar(@$files) > 0, 'Found files');

    my $mdef = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
    ok($mdef, 'Found protobuf_perl_test.TestMessage');
    is($mdef->full_name, 'protobuf_perl_test.TestMessage', 'Correct full name');

    my $f = $mdef->find_field_by_name('value');
    ok($f, 'Found "value" field');
    is($f->number, 1, 'Field number matches');
    is($f->type, 'int32', 'Field type is INT32');
};

subtest 'Pure-Perl Enum Lookup' => sub {
    my $pool = Protobuf::DescriptorPool->new();
    my $bin_data = path('t/data/test_descriptor.bin')->slurp_raw;
    $pool->add_serialized_file_descriptor_set($bin_data);

    my $enum = $pool->find_enum_by_name('protobuf_perl_test.TestEnum');
    ok($enum, 'Found protobuf_perl_test.TestEnum');

    my $val = $enum->find_value_by_name('TEST_ENUM_UNSPECIFIED');
    ok($val, 'Found enum value by name');
    is($val->number, 0, 'Enum value number matches');

    my $val2 = $enum->find_value_by_number(1);
    ok($val2, 'Found enum value by number');
    is($val2->name, 'TEST_ENUM_FIRST', 'Enum value name matches');
};

done_testing();
