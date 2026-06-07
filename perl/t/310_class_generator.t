use strict;
use warnings;
use Test::More;
use lib "t/lib";
use TestHelpers;
use Protobuf;
use Protobuf::DescriptorPool;
use Protobuf::ClassGenerator;

# 1. Load descriptors
my $pool = Protobuf::DescriptorPool->generated_pool;
my $desc_file = 't/data/test_descriptor.bin';
open my $fh, '<:raw', $desc_file or die "Can't open $desc_file: $!";
my $serialized = do { local $/; <$fh> };
close $fh;

my $files = $pool->add_serialized_file_descriptor_set($serialized);
ok($files, "Added descriptors from $desc_file");

my $file = $pool->find_file_by_name('google/protobuf/descriptor.proto');
# If descriptor.proto isn't in that set, let's look for a message we KNOW is there.
if (!$file) {
    vdiag("descriptor.proto not found in set, trying test.proto");
    $file = $pool->find_file_by_name('perl/t/c/test.proto') || $pool->find_file_by_name('t/c/test.proto') || $pool->find_file_by_name('test.proto');
}
ok($file, "Found a descriptor file to generate from");

# 2. Generate classes
Protobuf::ClassGenerator->generate_for_file($file);

# 3. Test a generated class
my $mdef = $file->get_top_level_message(0);
my $class = $mdef->perl_class_name();

ok($class->can('new'), "Class $class was generated");
ok($class->can('nested_string') || $class->can('value'), "Accessor exists");

my $msg = $class->new;
isa_ok($msg, $class);
isa_ok($msg, 'Protobuf::Message');

# 4. Test accessors
$msg->nested_string('TestValue');
is($msg->nested_string, 'TestValue', "Roundtrip for 'nested_string' works");

# 5. Test another class from the same file
my $class2 = 'Protobuf_perl_test::Test::TestMessage';
ok($class2->can('new'), "Class $class2 was generated");
ok($class2->can('repeated_int'), "Accessor 'repeated_int' exists (repeated)");

my $msg2 = $class2->new;
my $rep = $msg2->repeated_int;
isa_ok($rep, 'Protobuf::Internal::Repeated::Public');
push @$rep, 1, 2, 3;
is(scalar(@$rep), 3, "Repeated field populated");
is($rep->[1], 2, "Element 1 is correct");

done_testing();
