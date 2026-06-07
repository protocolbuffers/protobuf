package main;
use strict;
use warnings;
use Test::More;
use Protobuf;
plan skip_all => 'XS required for Arena tests' unless $Protobuf::HAS_XS;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

subtest 'descriptor pool and arena interaction' => sub {
    my $arena = Protobuf::Arena->new;
    ok($arena, 'Created arena');

    my $pool = TestHelpers->get_empty_pool();
    ok($pool, 'Created descriptor pool');

    my $last_file = TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');
    ok($last_file, 'Added serialized file descriptor set');

    my $message_def = $pool->find_message_by_name('protobuf_perl_test.TestMessage');
    ok($message_def, 'Found protobuf_perl_test.TestMessage in pool');
    isa_ok($message_def, 'Protobuf::Descriptor::MessageDef');

    # The MessageDef should stay alive even if we undef the pool because
    # the pool itself holds the underlying C memory, but the message_def
    # wrapper contains a weak reference to it (or rather, the Perl object cache
    # ties them together). Wait, how does the user API dictate this?
    # Right now, our wrapper objects have SV pointers. Let's make sure
    # we don't crash when passing these things around.

    is($message_def->name, 'TestMessage', 'Message name is correct');
    is($message_def->full_name, 'protobuf_perl_test.TestMessage', 'Message full_name is correct');

    # Verify we can find fields
    my $field = $message_def->find_field_by_name('value');
    ok($field, 'Found field "value"');
    isa_ok($field, 'Protobuf::Descriptor::Field');
    is($field->name, 'value', 'Field name is correct');

    # Verify Enum types
    my $enum_def = $pool->find_enum_by_name('protobuf_perl_test.TestEnum');
    ok($enum_def, 'Found enum protobuf_perl_test.TestEnum');
    isa_ok($enum_def, 'Protobuf::Descriptor::Enum');
    is($enum_def->name, 'TestEnum', 'Enum name is correct');
};

TODO: {
    local $TODO = 'Implement Descriptor-Pool Isolation Stress';
    ok(0, 'Overlapping symbol names in different pools are correctly isolated');
}

TODO: {
    local $TODO = 'Verify Cross-Interpreter Descriptor Migration';
    ok(0, 'Descriptors maintain pool identity when migrated between interpreters');
}

TODO: {
    local $TODO = 'Implement Real-Time Schema Integrity Monitor';
    ok(0, 'Mojo service can validate descriptor pool integrity against source');
}

done_testing();
