use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/wkt_descriptor.bin');

subtest 'struct and value operations' => sub {
    my $data = {
        foo => 'bar',
        baz => 123,
        qux => [1, 2, { a => 'b' }],
        sub => { x => 1 }
    };

    my $struct = Google::Protobuf::Struct::Struct->new();
    $struct->from_perl($data);

    my $out = $struct->to_perl();
    is_deeply($out, $data, 'Struct from_perl/to_perl roundtrip matches');

    # Check individual fields
    is($struct->fields->{foo}->string_value, 'bar', 'String value correct');
    is($struct->fields->{baz}->number_value, 123, 'Number value correct');

    # Check oneof
    is($struct->fields->{foo}->kind, 'string_value', 'Oneof kind is string_value');

    subtest 'memory profile' => sub {
        my $profile = $struct->memory_profile();
        ok($profile, 'Got memory profile');
        ok(exists $profile->{arena_bytes}, 'Has arena_bytes metric');
    };
};

TODO: {
    local $TODO = 'Implement Struct-Specific Memory Profiling';
    ok(0, 'Struct and ListValue objects provide detailed memory overhead reporting');
}

done_testing();
