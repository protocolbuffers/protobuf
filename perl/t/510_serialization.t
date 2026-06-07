use strict;
use warnings;
use Test::More;
use Protobuf;
use Protobuf::DescriptorPool;
use Protobuf::ClassGenerator;
use JSON::MaybeXS;

# 1. Load descriptors
my $pool = Protobuf::DescriptorPool->generated_pool;
my $desc_file = 't/data/test_descriptor.bin';
open my $fh, '<:raw', $desc_file or die "Can't open $desc_file: $!";
my $serialized = do { local $/; <$fh> };
close $fh;
$pool->add_serialized_file_descriptor_set($serialized);

# 2. Test to_perl
subtest 'to_perl conversion' => sub {
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->value(42);
    $msg->test_string("hello");
    push @{$msg->repeated_int}, 1, 2, 3;

    my $nested = Protobuf_perl_test::Test::NestedMessage->new();
    $nested->nested_string("deep");
    $msg->nested_message($nested);

    my $perl = $msg->to_perl();
    is($perl->{value}, 42, "Value correct in hash");
    is($perl->{test_string}, "hello", "String correct in hash");
    is_deeply($perl->{repeated_int}, [1, 2, 3], "Repeated field correct in hash");
    is($perl->{nested_message}->{nested_string}, "deep", "Nested message correct in hash");

    # Verify it's a regular hash, not tied
    ok(!tied(%$perl), "Result hash is NOT tied");
};

# 3. Test to_json / from_json
subtest 'JSON serialization' => sub {
    plan skip_all => 'JSON is not supported in PurePerl mode' unless $Protobuf::HAS_XS;
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->value(100);
    $msg->test_string("json test");

    my $json = $msg->to_json();
    like($json, qr/"value":\s*100/, "JSON contains value");
    like($json, qr/"testString":\s*"json test"/, "JSON contains string (camelCase check)");

    my $msg2 = Protobuf_perl_test::Test::TestMessage->from_json($json);
    is($msg2->value, 100, "from_json restored value");
    is($msg2->test_string, "json test", "from_json restored string");
};

# 4. Test to_text
subtest 'Text format serialization' => sub {
    plan skip_all => 'Text format is only stub in PurePerl mode' unless $Protobuf::HAS_XS;
    my $msg = Protobuf_perl_test::Test::TestMessage->new();
    $msg->value(7);

    my $text = $msg->to_text();
    like($text, qr/value:\s*7/, "Text format contains value");
};

done_testing();
