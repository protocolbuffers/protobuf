#include "t/c/upb-perl-test.h"
#include <setjmp.h>
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    {
        ENTER; SAVETMPS;
        dJMPENV;
        int ret;
        JMPENV_PUSH(ret);
        if (ret == 0) {
            croak("Test croak");
            JMPENV_POP;
        } else {
            JMPENV_POP;
            sv_setsv_mg(ERRSV, &PL_sv_undef);
        }
        FREETMPS; LEAVE;
    }

    test_perl_destroy(my_perl);
    return 0;
}
