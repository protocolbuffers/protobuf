package main;
use strict;
use warnings;
use Test::More;
use Protobuf::ClassGenerator;

subtest 'release preparation' => sub {
    my $docs = Protobuf::ClassGenerator->generate_docs();
    ok($docs, 'Got HTML documentation');
    like($docs, qr/<h1>Protobuf Documentation<\/h1>/, 'Docs contain title');
};

TODO: {
    local $TODO = 'Automated CPAN/CI/CD Pipeline';
    ok(0, 'ASan/TSan integrated into GitHub Actions');
}

TODO: {
    local $TODO = 'Include Embedded Benchmark Suite';
    ok(0, 'Benchmarks automatically run during installation');
}

done_testing();
