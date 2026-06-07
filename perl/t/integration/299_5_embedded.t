use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;
use Sys::Hostname;
use Time::HiRes qw(tv_interval gettimeofday);

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/compat_descriptor.bin');

subtest 'embedded messages and enums' => sub {
    my $severity = Apache2::Protobuf::EmbeddedError::Error::Severity->WARN;
    my $message  = 'Here is a warning';

    my $now   = [gettimeofday];
    my $error = Apache2::Protobuf::EmbeddedError::Error->new;

    $error->set_datetime(time());
    $error->set_severity($severity);
    $error->set_message($message);
    $error->set_hostname(hostname());
    $error->set_pid($$);

    # Include some stack frames
    for ( my $i = 0; $i < 3; $i++ ) {
        my ($pack, $file, $line) = caller($i);
        last unless $pack;

        my $frame = Apache2::Protobuf::EmbeddedError::Error::StackFrame->new;
        $frame->set_file($file);
        $frame->set_line($line);
        $error->add_trace($frame);
    }

    my $packed  = $error->serialize();
    ok(length($packed) > 0, 'Serialized embedded error');

    my $u = Apache2::Protobuf::EmbeddedError::Error->parse($packed);
    ok($u, 'Parsed embedded error');

    is($u->severity, $severity, 'Severity preserved');
    is($u->message, $message, 'Message preserved');
    is(scalar(@{$u->trace}), 3, 'Trace count preserved');
    is($u->trace->[0]->file, (caller(0))[1], 'First frame file preserved');
};

subtest 'constructor with nested structures' => sub {
    my $error = Apache2::Protobuf::EmbeddedError::Error->new({
        datetime => time(),
        severity => Apache2::Protobuf::EmbeddedError::Error::Severity->ERROR,
        message  => 'Fatal crash',
        hostname => 'localhost',
        pid      => 1234,
        trace    => [
            { file => 'foo.pl', line => 10 },
            { file => 'bar.pl', line => 20 },
        ]
    });

    ok($error, 'Created with nested HashRef');
    is(scalar(@{$error->trace}), 2, 'Nested trace count ok');
    is($error->trace->[1]->line, 20, 'Nested trace data ok');

    my $p = $error->serialize();
    my $u = Apache2::Protobuf::EmbeddedError::Error->parse($p);
    is($u->trace->[0]->file, 'foo.pl', 'Roundtrip with nested structures ok');
};

done_testing();
