use strict;
use warnings;
use Test::More;
use Protobuf;

# We do NOT use 'use lib "t/lib"' as per SOP.
# Instead, we rely on -It/lib being passed to the 'prove' command.
use TestHelpers;

subtest 'Check active engine' => sub {
    plan tests => 2;

    ok($Protobuf::HAS_XS, "XS engine is loaded!") 
        or diag("Error: XS failed to load, fell back to PurePerl");
    isa_ok(Protobuf->engine, 'Protobuf::Engine::XS', "Active engine");

    done_testing();
};

done_testing();
