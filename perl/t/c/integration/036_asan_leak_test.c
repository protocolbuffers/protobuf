#include "t/c/upb-perl-test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

// --- Mini XS Module for Leak Test ---
static HV *test_static_cache = NULL;

XS(XS_TestLeak_store) {
    dXSARGS;
    if (items != 2)
        croak("Usage: TestLeak::store(key, value)");

    const char *key = SvPV_nolen(ST(0));
    SV *value = ST(1);

    if (!test_static_cache)
        test_static_cache = newHV();

    SV *key_sv = newSVpvn(key, strlen(key));
    SV *val_ref = newRV_inc(value);

    if (!hv_store_ent(test_static_cache, key_sv, val_ref, 0)) {
        SvREFCNT_dec(val_ref);
        SvREFCNT_dec(key_sv); // Clean up if store failed
    }
    // BUG: This is the leak, from the original obj_cache issue
    SvREFCNT_dec(key_sv);
    XSRETURN_YES;
}

XS(XS_TestLeak_clear) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    if (test_static_cache) {
        hv_clear(test_static_cache);
    }
    XSRETURN_YES;
}

XS(XS_TestLeak_destroy_cache) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    if (test_static_cache) {
        SvREFCNT_dec(test_static_cache);
        test_static_cache = NULL;
    }
    XSRETURN_YES;
}

static void xs_init_testleak(pTHX) {
    newXS("TestLeak::store", XS_TestLeak_store, __FILE__);
    newXS("TestLeak::clear", XS_TestLeak_clear, __FILE__);
    newXS("TestLeak::destroy_cache", XS_TestLeak_destroy_cache, __FILE__);
}
// --- End Mini XS Module ---

// Child function for the HV key leak test
static void hv_key_leak_child(pTHX) {
    eval_pv("TestLeak::store('test_key', 123)", TRUE);
    if (SvTRUE(ERRSV)) {
      fprintf(stderr, "# CHILD: Eval failed: %s\n", SvPV_nolen(ERRSV));
    }
    eval_pv("TestLeak::clear()", TRUE); // This will not free the key
}

// Test function to demonstrate HV key leak by forking
static void test_hv_key_leak(pTHX) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        fail("HV Key Leak: Failed to create pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        fail("HV Key Leak: Failed to fork");
        return;
    } else if (pid == 0) {
        // Child process
        PerlInterpreter *child_perl = perl_alloc();
        perl_construct(child_perl);
        PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
        char *embedding[] = { (char*)"", (char*)"-e", "0", NULL };
        perl_parse(child_perl, xs_init_testleak, 3, embedding, NULL); // Load our mini XS
        perl_run(child_perl);

        close(pipefd[0]);    // Close unused read end
        dup2(pipefd[1], STDERR_FILENO); // Redirect stderr to pipe
        close(pipefd[1]);    // Close original write end

        hv_key_leak_child(aTHX);

        eval_pv("TestLeak::destroy_cache()", TRUE); // Clean up the cache HV itself
        perl_destruct(child_perl);
        perl_free(child_perl);
        exit(0); // Exit normally, ASan *should* make it non-zero due to the leak
    } else {
        // Parent process
        close(pipefd[1]);    // Close unused write end

        char child_stderr[8192] = {0};
        ssize_t bytes_read = read(pipefd[0], child_stderr, sizeof(child_stderr) - 1);
        close(pipefd[0]);

        if (bytes_read > 0) {
            cdiag("HV Key Leak Child stderr:\n%s", child_stderr);
        } else {
            cdiag("HV Key Leak Child stderr: (empty or read error)");
        }

        int status;
        waitpid(pid, &status, 0);

        ok(1, "HV Key Leak: Fork and wait completed");

        TODO") {
            // This assertion SHOULD fail, as the child exits 0
            ok(WIFEXITED(status) && WEXITSTATUS(status) != 0, "EXPECT FAIL: HV Key Leak: Child exited non-zero");
            // This assertion SHOULD fail, as no leak is reported in the child's stderr
            like(child_stderr, "Direct leak of", "EXPECT FAIL: HV Key Leak: ASan detected leak in child");
        }
    }
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(3); // Adjusted plan for the forking test

    test_hv_key_leak(aTHX);

    test_perl_destroy(my_perl);
    return 0;
}