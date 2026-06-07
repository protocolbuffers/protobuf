use strict;
use warnings;
use Test::More;

subtest 'protoc plugin existence' => sub {
    my $plugin = 'bin/protoc-gen-perl-pb';
    ok(-f $plugin, 'Plugin file exists');
    ok(-r $plugin, 'Plugin file is readable');
};

TODO: {
    local $TODO = 'Implement C-Layer based code generation';
    ok(0, 'Plugin generates high-performance Perl modules using upb descriptors');
}

TODO: {
    local $TODO = 'Support Mojo/Coro service stubs';
    ok(0, 'Plugin generates non-blocking service stubs for modern event loops');
}

TODO: {
    local $TODO = 'Embed serialized descriptors';
    ok(0, 'Generated modules contain self-contained binary descriptors');
}

done_testing();
