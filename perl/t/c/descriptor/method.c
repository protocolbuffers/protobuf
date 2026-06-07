#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor.h"

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(1);

    TODO {
        ok(0, "method unit tests");
    }

    test_perl_destroy(my_perl);
    return 0;
}
