use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/compat_descriptor.bin');

subtest 'required fields error handling' => sub {
    foreach (1 .. 3) {
        my $msg = Protobuf_perl_error::Error::Error->new({ field3 => 'foo' });
        eval {
            my $p = $msg->serialize();
        };
        ok($@, "Attempt $_ to serialize message with missing required fields throws exception");
        like($@, qr/Missing required fields/, "Error message correctly identifies missing fields in attempt $_");
    }
};

subtest 'partial initialization error' => sub {
    my $msg = Protobuf_perl_error::Error::Error->new({ field1 => 123 });
    eval { $msg->serialize() };
    ok($@, "Missing field2 throws exception");

    $msg->set_field2("present");
    my $p;
    eval { $p = $msg->serialize() };
    ok(!$@, "All required fields present, serialization succeeds") or diag($@);
    ok(length($p) > 0, "Serialized data is non-empty");
};

done_testing();
