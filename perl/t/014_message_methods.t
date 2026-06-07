use strict;
use warnings;
use Test::More;
use Protobuf;
use Protobuf::DescriptorPool;
use Path::Tiny;

# Use the compat_descriptor.bin for tests
my $pool = Protobuf::DescriptorPool->generated_pool;
my $bin = path('t/data/compat_descriptor.bin')->slurp_raw;
$pool->add_serialized_file_descriptor_set($bin);

ok(String::StringBytes::Bytes->can('new'), 'String::StringBytes::Bytes can new');

subtest 'fields()' => sub {
    my $msg = String::StringBytes::Bytes->new(
        v_string => 'hello',
    );

    my @fields = $msg->fields;
    is(scalar(@fields), 1, 'One field is set');

    is($fields[0]->name, 'v_string', 'Field name matches');

    $msg->v_bytes('world');
    @fields = $msg->fields;
    is(scalar(@fields), 2, 'Two fields are set');
};

subtest 'merge_from()' => sub {
    my $msg1 = String::StringBytes::Bytes->new(
        v_string => 'hello',
    );
    my $msg2 = String::StringBytes::Bytes->new(
        v_bytes => 'world',
    );

    $msg1->merge_from($msg2);
    is($msg1->v_string, 'hello', 'Old field preserved');
    is($msg1->v_bytes, 'world', 'New field merged');

    my $msg3 = String::StringBytes::Bytes->new(
        v_string => 'overwritten',
    );
    $msg1->merge_from($msg3);
    is($msg1->v_string, 'overwritten', 'Existing field overwritten by merge');

    subtest 'repeated fields' => sub {
        my $msg_rep1 = Protobuf::Types::Types->new(
            rep_int32 => [1, 2],
        );
        my $msg_rep2 = Protobuf::Types::Types->new(
            rep_int32 => [3, 4],
        );
        $msg_rep1->merge_from($msg_rep2);
        is_deeply([@{$msg_rep1->rep_int32}], [1, 2, 3, 4], 'Repeated fields are concatenated during merge');
    };
};

subtest 'copy_from()' => sub {
    my $msg1 = String::StringBytes::Bytes->new(
        v_string => 'hello',
    );
    my $msg2 = String::StringBytes::Bytes->new(
        v_bytes => 'world',
    );

    $msg1->copy_from($msg2);
    ok(!$msg1->has_v_string, 'Old field cleared');
    is($msg1->v_bytes, 'world', 'New field copied');
};

subtest 'parse_from()' => sub {
    my $msg = String::StringBytes::Bytes->new(
        v_string => 'hello',
    );
    my $other = String::StringBytes::Bytes->new(
        v_bytes => 'world',
    );
    my $bin = $other->serialize;

    $msg->parse_from($bin);
    is($msg->v_string, 'hello', 'Existing field preserved during parse_from');
    is($msg->v_bytes, 'world', 'New field merged during parse_from');
};

done_testing;
