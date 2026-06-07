use strict;
use warnings;
use Test::More;
use Protobuf::Engine::PurePerl::DescriptorParser;
use Path::Tiny;

my $bin_data = path('t/data/test_descriptor.bin')->slurp_raw;
my $files = Protobuf::Engine::PurePerl::DescriptorParser->parse_file_descriptor_set($bin_data);

# Debug output
diag("Files found: " . join(", ", map { $_->{name} } @$files));

ok(ref($files) eq 'ARRAY', 'Parsed file descriptor set');
ok(scalar(@$files) > 0, 'Found at least one file descriptor');

my $test_proto = (grep { $_->{name} =~ /test\.proto/ } @$files)[0];
ok($test_proto, 'Found test.proto');
is($test_proto->{package}, 'protobuf_perl_test', 'Correct package name');

my $msg = (grep { $_->{name} eq 'TestMessage' } @{$test_proto->{message_type}})[0];
ok($msg, 'Found TestMessage');
is($msg->{name}, 'TestMessage', 'Correct message name');

my $val_field = (grep { $_->{name} eq 'value' } @{$msg->{field}})[0];
ok($val_field, 'Found "value" field');
is($val_field->{number}, 1, 'Field number is 1');
is($val_field->{type}, 5, 'Field type is 5 (INT32)');

my $enum = (grep { $_->{name} eq 'TestEnum' } @{$test_proto->{enum_type}})[0];
ok($enum, 'Found TestEnum');
my $val0 = $enum->{value}[0];
is($val0->{name}, 'TEST_ENUM_UNSPECIFIED', 'Enum value name correct');
is($val0->{number}, 0, 'Enum value number correct');

done_testing();
