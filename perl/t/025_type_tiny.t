use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Protobuf::ClassGenerator;

# Load descriptors
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin', 't/data/wkt_descriptor.bin');

{
    package My::App;
    use Moo;
    use Types::Standard qw( InstanceOf HashRef );
    use Type::Utils qw( coerce );

    # Define a coercion from HashRef to our Message class
    my $TestMsgType = InstanceOf['Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2'];
    my $CoercedTestMsg = $TestMsgType->plus_coercions(
        HashRef, sub {
            my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
            $msg->from_perl($_);
            return $msg;
        }
    );

    has 'message' => (
        is       => 'ro',
        isa      => $CoercedTestMsg,
        coerce   => 1,
    );
}

subtest 'Moo InstanceOf constraint' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg->set_optional_int32(42);

    my $app = My::App->new(message => $msg);
    ok($app, 'App created with Protobuf object');
    is($app->message->optional_int32, 42, 'Message object preserved');
    isa_ok($app->message, 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2');
};

subtest 'Type::Tiny coercion from HashRef' => sub {
    my $app = My::App->new(message => { optional_int32 => 99, optional_string => "coerced" });
    ok($app, 'App created with HashRef (coerced)');
    isa_ok($app->message, 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2');
    is($app->message->optional_int32, 99, 'Coerced value correct');
    is($app->message->optional_string, 'coerced', 'Coerced string correct');
};

subtest 'Nested HashRef support' => sub {
    my $app = My::App->new(message => {
        optional_int32 => 1,
        optional_nested_message => {
            a => 123
        }
    });

    ok($app, 'App created with nested HashRef');
    is($app->message->optional_nested_message->a, 123, 'Nested data correctly populated');
};

subtest 'to_perl deep conversion' => sub {
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg->set_optional_int32(123);
    $msg->set_optional_string("deep");
    push @{$msg->repeated_int32}, 1, 2, 3;
    $msg->map_int32_int32->{5} = 10;

    my $sub = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2::NestedMessage->new();
    $sub->set_a(999);
    $msg->set_optional_nested_message($sub);

    my $data = $msg->to_perl();
    is_deeply($data, {
        optional_int32 => 123,
        optional_string => "deep",
        repeated_int32 => [1, 2, 3],
        map_int32_int32 => { 5 => 10 },
        optional_nested_message => {
            a => 999
        }
    }, 'Deep to_perl HashRef is correct');
};

subtest 'WKT integration with Type::Tiny' => sub {
    # 1. google.protobuf.Struct
    my $struct = Google::Protobuf::Struct::Struct->new();
    $struct->from_perl({ a => 1, b => { c => 3 } });

    my $perl = $struct->to_perl();
    is_deeply($perl, { a => 1, b => { c => 3 } }, 'Struct deep conversion matches');

    # 2. google.protobuf.Any
    my $msg = Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2->new();
    $msg->set_optional_int32(42);

    my $any = Google::Protobuf::Any::Any->new();
    $any->pack($msg);

    my $unpacked = $any->unpack();
    isa_ok($unpacked, 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2');
    is($unpacked->optional_int32, 42, 'Any unpacked correctly');
};

TODO: {
    local $TODO = 'Implement C-to-Type::Tiny Compiled Validation';
    ok(0, 'Protobuf validation logic is exported as compiled subroutines for Type::Tiny');
}

TODO: {
    local $TODO = 'Generate Type Libraries for every .proto file';
    ok(0, 'Types::Protobuf module is automatically created with message constraints');
}

TODO: {
    local $TODO = 'Implement Intelligent Union-Type (Oneof) Coercion';
    ok(0, 'HashRef keys intelligently populate oneof branches during coercion');
}

TODO: {
    local $TODO = 'Implement Zero-Copy Type Coercion';
    ok(0, 'Protobuf objects coerced to other Moo types without deep-copying shared memory');
}

TODO: {
    local $TODO = 'Implement Self-Healing Type Integrity background auditing';
    ok(0, 'Integrated auditor detects and reports type mismatches in reified object trees');
}

TODO: {
    local $TODO = 'Implement Automated Type-Library Optimization via AOT compilation';
    ok(0, 'Type libraries for messages are pre-compiled to XS for maximum performance');
}

subtest 'type library generation' => sub {
    my $file = $pool->find_file_by_name('perl/t/c/test.proto') || $pool->find_file_by_name('test.proto');
    my $lib_code = Protobuf::ClassGenerator->generate_type_library($file);
    vdiag("Generated Type Library:\n$lib_code");

    ok($lib_code, 'Got type library code');
    like($lib_code, qr/package Protobuf_perl_test::Test::Types;/, 'Correct package name');
    like($lib_code, qr/use Type::Library/, 'Code contains Type::Library usage');
    like($lib_code, qr/declare 'TestMessage'/, 'Defines TestMessage type');
    like($lib_code, qr/where \{ \$_->validate \}/, 'Includes native validation check');
    like($lib_code, qr/coerce 'TestMessage'/, 'Includes coercion logic');
};

done_testing();
