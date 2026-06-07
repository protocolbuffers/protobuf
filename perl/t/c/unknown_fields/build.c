#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
// #include "xs/unknown_fields/build.h" // Missing file

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(1);

    TODO {
        ok(0, "Unknown fields building from raw data tested");
    }

    test_perl_destroy(my_perl);
    return 0;
}
