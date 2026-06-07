package main;
use strict;
use warnings;
use Test::More;
use Protobuf;
plan skip_all => 'XS required for tmpfs IPC integration tests' unless $Protobuf::HAS_XS;
use lib "t/lib";
use TestHelpers;
use POSIX qw( mkfifo );
use File::Temp qw( tempdir );
use Protobuf::Arena;

my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/test_descriptor.bin');

my $tmpdir = tempdir( CLEANUP => 1 );
my $shm_file = "$tmpdir/proto_shm";
my $fifo = "$tmpdir/proto_fifo";

mkfifo($fifo, 0600) or die "mkfifo failed: $!";

my $msg_class = 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2';
my $proto_name = 'protobuf_test_messages.proto2.TestAllTypesProto2';

my $pid = fork();
die "fork failed" unless defined $pid;

if ($pid == 0) {
    # Child Process: Consumer
    # 1. Wait for signal from FIFO
    open my $fh_fifo, '<', $fifo or exit 10;
    my $line = <$fh_fifo>;
    close $fh_fifo;

    chomp $line;
    my ($path, $size, $offset) = split /:/, $line;

    # 2. Attach to the same SHM arena
    my $arena = eval { Protobuf::Arena->attach_tmpfs($path, $size) };
    exit 1 if $@ || !$arena;

    # 3. Reify the message (Zero-Copy)
    my $msg = eval { $arena->attach_message($proto_name, $offset) };
    exit 2 if $@ || !$msg;
    exit 3 unless $msg->isa($msg_class);

    # 4. Validate content
    if ($msg->optional_int32 == 12345 && $msg->optional_string eq "shm_zero_copy") {
        exit 0;
    } else {
        exit 4;
    }
} else {
    # Parent Process: Producer
    # 1. Create tmpfs arena
    my $size = 1024 * 1024;
    my $arena = Protobuf::Arena->new_tmpfs($shm_file, $size);
    ok($arena, "Parent created tmpfs arena at $shm_file");

    # 2. Create message in that arena
    my $msg = $msg_class->new(arena => $arena);
    $msg->set_optional_int32(12345);
    $msg->set_optional_string("shm_zero_copy");

    # 3. Get offset
    my $ptr_iv = $msg->{_upb_ptr};
    my $offset = $arena->get_offset($ptr_iv);
    ok($offset > 0, "Got message offset: $offset");

    # 4. Signal via FIFO
    open my $fh_fifo, '>', $fifo or die $!;
    print $fh_fifo "$shm_file:$size:$offset\n";
    close $fh_fifo;

    # 5. Wait for child
    waitpid($pid, 0);
    my $exit_code = $? >> 8;

    is($exit_code, 0, 'Child verified zero-copy message content successfully')
        or diag("Child failed with exit code: $exit_code (1=attach fail, 2=reify fail, 3=isa fail, 4=content fail, 10=fifo fail)");

    unlink($shm_file) if -f $shm_file;
}

done_testing();
