use strict;
use warnings;
use Test::More;
use Protobuf::Arena;
use lib "t/lib";
use TestHelpers;

# Load descriptors into the generated pool
my $pool = TestHelpers->get_generated_pool();
TestHelpers->load_test_protos($pool, 't/data/compat_descriptor.bin');

subtest 'package and nested naming' => sub {
    my $m1 = Local::Messages::Messages::One->new({ id1 => 1 });
    my $m2 = Local::Messages::Messages::Two->new({ id2 => 2 });
    my $m3 = Local::Messages::Messages::Three->new({ id3 => 3 });

    my $p1 = $m1->serialize();
    my $p2 = $m2->serialize();
    my $p3 = $m3->serialize();

    my $n1 = Local::Messages::Messages::One->parse($p1);
    my $n2 = Local::Messages::Messages::Two->parse($p2);
    my $n3 = Local::Messages::Messages::Three->parse($p3);

    is($n1->id1, 1, 'id1 ok');
    is($n2->id2, 2, 'id2 ok');
    is($n3->id3, 3, 'id3 ok');
};

done_testing();
