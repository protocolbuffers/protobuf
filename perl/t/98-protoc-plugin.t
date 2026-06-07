#!/usr/bin/env perl

use strict;
use warnings;
use Test::More;
use Protobuf;

# Mock Google::Auth and Google::gRPC::Client to make test hermetic and run on clean public checkouts
BEGIN {
    package Google::Auth;
    $INC{'Google/Auth.pm'} = 1;
    sub default { bless {}, shift }
    sub get_token { 'mock_token' }

    package Google::gRPC::Client;
    $INC{'Google/gRPC/Client.pm'} = 1;
    sub new { my $class = shift; bless {@_}, $class }
}

use File::Path qw(make_path remove_tree);
use File::Temp qw(tempdir);
use Capture::Tiny qw(capture);
use Cwd qw(cwd);

my $cwd = cwd();

my $protoc = 'protoc'; # Assumes protoc is in PATH
my $plugin = -x './protoc-gen-perl-pb' ? './protoc-gen-perl-pb'
           : -x 'bin/protoc-gen-perl-pb' ? 'bin/protoc-gen-perl-pb'
           : './protoc-gen-perl-pb';

unless (-x $plugin) {
    plan skip_all => 'Plugin protoc-gen-perl-pb not found (please run: make build_protoc_plugin)';
}

ok(1, "This test file runs");

my $tmpdir = tempdir(CLEANUP => 1);
my $proto_dir = "$tmpdir/protos";
my $out_dir = "$tmpdir/lib";
make_path($proto_dir);
make_path($out_dir);

# Create a dependency proto file
my $dep_proto_file = "$proto_dir/dep.proto";
open my $dfh, '>', $dep_proto_file or die "Could not open $dep_proto_file: $!";
print $dfh <<'EOF';
syntax = "proto3";
package mypackage.dep;

message DepMessage {
  int32 dep_field = 1;
}
EOF
close $dfh;

# Create a simple proto file
my $proto_file = "$proto_dir/test.proto";
open my $fh, '>', $proto_file or die "Could not open $proto_file: $!";
print $fh <<'EOF';
syntax = "proto3";
package mypackage;

import "dep.proto";

message MyMessage {
  string my_field = 1;
  MyEnum my_enum = 2;
  message NestedMessage {
    int32 nested_field = 1;
  }
  NestedMessage nested_msg = 3;
  mypackage.dep.DepMessage dep_msg = 4;
}

enum MyEnum {
  MY_ENUM_UNSPECIFIED = 0;
  MY_ENUM_VALUE_A = 1;
  MY_ENUM_VALUE_B = 2;
}

service MyService {
  rpc DoStuff(MyMessage) returns (MyMessage);
}
EOF
close $fh;

# Run protoc with the plugin
my @protoc_args = (
    $protoc,
    "-I=$proto_dir",
    "--plugin=protoc-gen-perl-pb=$cwd/$plugin",
    "--perl-pb_out=$out_dir",
    "--perl-pb_opt=embed_descriptors,generate_services",
    "test.proto",
    "dep.proto",
);

my ($stdout, $stderr, $exit) = capture { system(@protoc_args) };

diag "protoc stdout:
$stdout" if $stdout;
diag "protoc stderr:
$stderr" if $stderr;

is($exit, 0, 'protoc command exited successfully');

my $expected_pm = "$out_dir/Mypackage/Test.pm";
ok(-f $expected_pm, "Generated file $expected_pm exists");

my $expected_dep_pm = "$out_dir/Mypackage/Dep/Dep.pm";
ok(-f $expected_dep_pm, "Generated dep file $expected_dep_pm exists");

my $expected_types_pm = "$out_dir/Mypackage/Test/Types.pm";
ok(-f $expected_types_pm, "Generated types file $expected_types_pm exists");

# Add generated lib to @INC
eval { unshift @INC, $out_dir; };
if ($@) {
    fail("Failed to add $out_dir to \@INC");
}

subtest 'Test Generated Code' => sub {
    eval {
        require Mypackage::Test;
        require Mypackage::Dep::Dep;
    };
    ok(!$@, 'Successfully loaded generated modules') or diag $@;
    return if $@;

    is(Mypackage::Test::MyEnum::MY_ENUM_VALUE_A(), 1, 'Enum MY_ENUM_VALUE_A ok');
    is(Mypackage::Test::MyEnum::MY_ENUM_VALUE_B(), 2, 'Enum MY_ENUM_VALUE_B ok');

    my $msg;
    eval {
        $msg = Mypackage::Test::MyMessage->new(
            my_field   => 'hello',
            my_enum    => 'MY_ENUM_VALUE_B',
            nested_msg => { nested_field => 123 },
            dep_msg    => { dep_field    => 456 },
        );
    };
    ok(!$@, 'Mypackage::Test::MyMessage instantiated with nested HashRefs ok') or diag $@;
    return if $@;

    is($msg->my_field(), 'hello', 'my_field ok');
    is($msg->my_enum(), Mypackage::Test::MyEnum::MY_ENUM_VALUE_B(), 'my_enum ok');

    my $nested = $msg->nested_msg;
    isa_ok($nested, 'Mypackage::Test::MyMessage::NestedMessage', 'nested_msg is correct type');
    is($nested->nested_field(), 123, 'nested_field ok');

    my $dep = $msg->dep_msg;
    isa_ok($dep, 'Mypackage::Dep::Dep::DepMessage', 'dep_msg is correct type');
    is($dep->dep_field(), 456, 'dep_field ok');

    # Check for service stubs
    my $client_class = 'Mypackage::Test::MyServiceClient';
    ok($client_class->can('new'), 'MyServiceClient class generated');
    my $client = $client_class->new(target => 'localhost:443');
    isa_ok($client, 'Mypackage::Test::MyServiceClient');
    ok($client->can('do_stuff'), 'Client has do_stuff method');
};

subtest 'Descriptor Pool Check' => sub {
    eval {
        require Protobuf::DescriptorPool;
        my $pool = Protobuf::DescriptorPool->generated_pool();
        ok($pool->find_message_by_name('mypackage.MyMessage'), 'Found MyMessage in pool');
        ok($pool->find_message_by_name('mypackage.MyMessage.NestedMessage'), 'Found NestedMessage in pool');
        ok($pool->find_message_by_name('mypackage.dep.DepMessage'), 'Found DepMessage in pool');
        ok($pool->find_enum_by_name('mypackage.MyEnum'), 'Found MyEnum in pool');
        SKIP: {
            skip "Services are not supported in PurePerl mode", 1 unless $Protobuf::HAS_XS;
            ok($pool->find_service_by_name('mypackage.MyService'), 'Found MyService in pool');
        }
    };
    ok(!$@, "Descriptor pool tests executed") or diag $@;
};

done_testing();
