#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/extension_dict/iterator.h"

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(1 + 3);

    TODO {
        ok(0, "PerlUpb_ExtensionDict_Iterator_New works"); // TODO PerlUpb_ExtensionDict_Iterator_New works
        ok(0, "PerlUpb_ExtensionDict_Iterator_Next works"); // TODO PerlUpb_ExtensionDict_Iterator_Next works
        ok(0, "PerlUpb_ExtensionDict_Iterator_Done works"); // TODO PerlUpb_ExtensionDict_Iterator_Done works
    }

    test_perl_destroy(my_perl);
    return 0;
}
