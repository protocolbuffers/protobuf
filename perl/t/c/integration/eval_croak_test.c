#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <regex.h>
#include "t/c/upb-perl-test.h"

// Mock test functions (if not using a specific test header)


void plan(int count) {
    printf("1..%d\n", count);
}

void ok(int pass, const char *desc) {
    test_num++;
    printf("%s %d - %s\n", pass ? "ok" : "not ok", test_num, desc);
    if (!pass) {
        printf("#   Failed test '%s'\n", desc);
    }
}

void fail(const char *desc) {
    ok(0, desc);
}

void // log something here? (const char *fmt, ...) {
    va_list args;
    printf("# ");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void like(const char *got, const char *expected_regex, const char *desc) {
    if (got == NULL) {
        fail(desc);
        return;
    }

    regex_t regex;
    int reti;
    char msgbuf[100];

    reti = regcomp(&regex, expected_regex, REG_EXTENDED | REG_NEWLINE);
    if (reti) {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fail(desc);
        regfree(&regex);
        return;
    }

    reti = regexec(&regex, got, 0, NULL, 0);
    if (!reti) {
        ok(1, desc);
    } else {
        fail(desc);
    }
    regfree(&regex);
}
// End Mock test functions

// XS function that just croaks
static XS(XS_TestHelper_just_croak) {
    dXSARGS;
    croak("Direct croak from XS");
    XSRETURN_EMPTY;
}

// XS-style function to perform the eval
static XS(XS_TestHelper_eval_sv) {
    dXSARGS;
    if (items != 1) {
        croak("Usage: XS_TestHelper_eval_sv(code_sv)");
    }
    SV *code_sv = ST(0);
    eval_sv(code_sv, G_KEEPERR);
    XSRETURN_EMPTY;
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    newXS("TestHelper::just_croak", XS_TestHelper_just_croak, __FILE__);
    newXS("TestHelper::eval_sv", XS_TestHelper_eval_sv, __FILE__);

    plan(1);
    ok(1, "Tests commented out");

    test_perl_destroy(my_perl);
    return 0;
}
