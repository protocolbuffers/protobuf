use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;

subtest 'load and explore descriptors' => sub {
    my $pool = TestHelpers->get_empty_pool();

    my $files = TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');
    ok($files, 'Added descriptor set');
    my $last_file = $files->[-1];
    isa_ok($last_file, 'Protobuf::Descriptor::File');
    is($last_file->get_package, 'protobuf_perl_test', 'Correct package');

    subtest 'message descriptors' => sub {
        my $mdef = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
        ok($mdef, 'Found protobuf_perl_test.TestMessage');
        is($mdef->name, 'TestMessage', 'Correct name');

        my $f = $mdef->find_field_by_name('value');
        ok($f, 'Found field by name');
        is($f->number, 1, 'Correct number');
        is($f->type, 'int32', 'Type is int32');
    };

    subtest 'enum descriptors' => sub {
        my $edef = $pool->find_enum_by_name('protobuf_perl_test.TestEnum');
        ok($edef, 'Found protobuf_perl_test.TestEnum');
        is($edef->name, 'TestEnum', 'Correct name');
        is($edef->value_count, 3, 'Correct value count');
    };
};

TODO: {
    local $TODO = 'Implement Direct-to-Native Reflection';
    ok(0, 'Internal tasks can access C-level defs bypassing Perl wrappers');
}

TODO: {
    local $TODO = 'Implement Descriptor-Level Memory Profile';
    ok(0, 'Each descriptor subclass provides memory overhead reporting');
}

TODO: {
    local $TODO = 'Implement SIMD-Accelerated Descriptor Name Hashing';
    ok(0, 'High-speed hardware-accelerated name resolution verified');
}

TODO: {
    local $TODO = 'Implement Zero-Copy Descriptor Metadata Access';
    ok(0, 'Metadata retrieved from descriptors without redundant allocations');
}

TODO: {
    local $TODO = 'Implement Self-Healing Descriptor Identity';
    ok(0, 'Stable object identity maintained across repeated pool lookups');
}

done_testing();
