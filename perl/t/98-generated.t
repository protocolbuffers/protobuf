use strict;
use warnings;
use Test::More;
use File::Temp qw(tempdir);
use File::Spec;
use Protobuf;

# Mock Google::Auth and Google::gRPC::Client to make test hermetic
package Google::Auth;
$INC{'Google/Auth.pm'} = 1;
sub get_application_default { bless {}, shift }
sub default { bless {}, shift }
sub fetch_token { 'mock_token' }
sub get_token { 'mock_token' }

package Google::gRPC::Client;
$INC{'Google/gRPC/Client.pm'} = 1;
sub new { my $class = shift; bless {@_}, $class }

package main;

# Find protoc and our plugin
my $protoc = `which protoc`;
chomp $protoc;
plan skip_all => 'protoc not found' unless $protoc;

my $plugin_rel = -x 'protoc-gen-perl-pb' ? 'protoc-gen-perl-pb'
               : -x 'bin/protoc-gen-perl-pb' ? 'bin/protoc-gen-perl-pb'
               : undef;

plan skip_all => 'Plugin protoc-gen-perl-pb not found (please run: make build_protoc_plugin)' unless defined $plugin_rel;

my $plugin = File::Spec->rel2abs($plugin_rel);

my $tmpdir = tempdir(CLEANUP => 1);
my $proto_dir = 't/protos';

subtest 'generate String::StringBytes::Bytes' => sub {
    my $out_dir = tempdir(DIR => $tmpdir, CLEANUP => 1);
    my $cmd = "$protoc --plugin=protoc-gen-perl-pb=$plugin --perl-pb_out=embed_descriptors=true:$out_dir --proto_path=$proto_dir $proto_dir/string_bytes.proto 2>&1";
    my $output = `$cmd`;
    is($?, 0, 'protoc executed successfully') or diag($output);

    my $pm_file = File::Spec->catfile($out_dir, 'String', 'StringBytes.pm');
    ok(-f $pm_file, 'Generated String/StringBytes.pm exists');

    # Load the generated module
    unshift @INC, $out_dir;
    require String::StringBytes;

    ok(String::StringBytes::Bytes->can('new'), 'String::StringBytes::Bytes class generated');
    my $msg = String::StringBytes::Bytes->new(v_string => 'hello');
    is($msg->v_string, 'hello', 'Message works as expected');

    my $bin = $msg->serialize;
    my $msg2 = String::StringBytes::Bytes->parse($bin);
    is($msg2->v_string, 'hello', 'Roundtrip works');
};

subtest 'generate ProtobufTest::Types' => sub {
    my $out_dir = tempdir(DIR => $tmpdir, CLEANUP => 1);
    my $cmd = "$protoc --plugin=protoc-gen-perl-pb=$plugin --perl-pb_out=embed_descriptors=true:$out_dir --proto_path=$proto_dir $proto_dir/types.proto 2>&1";
    my $output = `$cmd`;
    is($?, 0, 'protoc executed successfully') or diag($output);

    my $pm_file = File::Spec->catfile($out_dir, 'Protobuf', 'Types.pm');
    ok(-f $pm_file, 'Generated Protobuf/Types.pm exists');

    # Load the generated module
    unshift @INC, $out_dir;
    require Protobuf::Types;

    ok(Protobuf::Types::Types->can('new'), 'Protobuf::Types::Types class generated');
    my $msg = Protobuf::Types::Types->new(opt_int32 => 123);
    is($msg->opt_int32, 123, 'Scalar field works');

    ok(Protobuf::Types::Types::Enum->can('value1'), 'Nested enum generated');
    is(Protobuf::Types::Types::Enum->value1(), 1, 'Enum value matches');
};

subtest 'generate service client stubs' => sub {
    my $out_dir = tempdir(DIR => $tmpdir, CLEANUP => 1);
    my $cmd = "$protoc --plugin=protoc-gen-perl-pb=$plugin --perl-pb_out=embed_descriptors=true:$out_dir --proto_path=$proto_dir $proto_dir/service.proto 2>&1";
    my $output = `$cmd`;
    is($?, 0, 'protoc executed successfully') or diag($output);

    my $pm_file = File::Spec->catfile($out_dir, 'Google', 'Cloud', 'Test', 'V1', 'Service.pm');
    ok(-f $pm_file, 'Generated Google/Cloud/Test/V1/Service.pm exists');

    # Load the generated module
    unshift @INC, $out_dir;
    require Google::Cloud::Test::V1::Service;

    ok(Google::Cloud::Test::V1::Service::HelloRequest->can('new'), 'HelloRequest message class generated');
    ok(Google::Cloud::Test::V1::Service::HelloResponse->can('new'), 'HelloResponse message class generated');

    my $client_class = 'Google::Cloud::Test::V1::Service::TestServiceClient';
    ok($client_class->can('new'), 'TestServiceClient class generated');

    # Let's inspect if client attributes and methods are present
    my $client = $client_class->new(target => 'test.googleapis.com:443');
    is($client->target, 'test.googleapis.com:443', 'Client has target attribute');
    ok($client->can('say_hello'), 'Client has say_hello method');
};

done_testing;
