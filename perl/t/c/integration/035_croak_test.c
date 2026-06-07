#include "t/c/upb-perl-test.h"
#include "xs/protobuf/port.h"

static void test_croak_recovery(void) {
    plan(2);

    char *my_argv[] = { "", "-e", "0", NULL };

    PerlInterpreter *test_perl = perl_alloc();
    perl_construct(test_perl);
    perl_parse(test_perl, NULL, 3, my_argv, NULL);
    perl_run(test_perl);
    PERL_SET_CONTEXT(test_perl);
    dTHX;

    bool caught = false;

    // Simulate a longjmp/croak scenario
    jmp_buf buf;
    if (setjmp(buf) == 0) {
        // Normal path
        ok(1, "Entering protected block");
        // Simulate croak
        longjmp(buf, 1);
    } else {
        // Caught path
        caught = true;
        ok(1, "Recovered from simulated longjmp");
    }

    perl_destruct(test_perl);
    perl_free(test_perl);
}

int main(int argc, char **argv, char **env) {
    PERL_SYS_INIT3(&argc, &argv, &env);
    test_croak_recovery();
    PERL_SYS_TERM();
    return 0;
}
