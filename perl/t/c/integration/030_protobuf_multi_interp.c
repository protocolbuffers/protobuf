#include "t/c/upb-perl-test.h"
#include "xs/protobuf/registry.h"
#include "xs/protobuf/obj_cache.h"

static void test_multi_interp(void) {
    plan(4);

    char *my_argv[] = { "", "-e", "0", NULL };

    cdiag("Initializing test_perl1");
    PerlInterpreter *test_perl1 = perl_alloc();
    perl_construct(test_perl1);
    perl_parse(test_perl1, NULL, 3, my_argv, NULL);
    perl_run(test_perl1);
    PERL_SET_CONTEXT(test_perl1);
    {
        dTHX;
        PerlUpb_Registry_Init(aTHX);
        PerlUpb_ObjCache_Init(aTHX);
        PerlUpb_Registry* reg1 = PerlUpb_Registry_Get(aTHX);
        void* dummy_ptr = (void*)0x1234;

        // We need to keep a strong reference to the object so it stays in cache
        SV* obj1 = newSViv(42);
        SV* strong_rv = newRV_inc(obj1);
        // Store it in a global scalar so it doesn't get collected
        sv_setsv(get_sv("main::keepalive", GV_ADD), strong_rv);

        PerlUpb_ObjCache_Add(aTHX_ dummy_ptr, strong_rv);

        cdiag("Initializing test_perl2");
        PerlInterpreter *test_perl2 = perl_alloc();
        perl_construct(test_perl2);
        perl_parse(test_perl2, NULL, 3, my_argv, NULL);
        perl_run(test_perl2);
        PERL_SET_CONTEXT(test_perl2);
        {
            dTHX;
            PerlUpb_Registry_Init(aTHX);
            PerlUpb_ObjCache_Init(aTHX);
            PerlUpb_Registry* reg2 = PerlUpb_Registry_Get(aTHX);

            ok(reg1 != reg2, "Registries are isolated across interpreters");

            SV* cached = PerlUpb_ObjCache_Get(aTHX_ dummy_ptr);
            ok(cached == NULL, "Cache is isolated: perl2 cannot see perl1 entries");
        }

        cdiag("Switching back to test_perl1");
        PERL_SET_CONTEXT(test_perl1);
        {
            dTHX;
            SV* cached = PerlUpb_ObjCache_Get(aTHX_ dummy_ptr);
            ok(cached != NULL, "Cache persist in perl1");
            if (cached) SvREFCNT_dec(cached);
        }

        cdiag("Cleaning up test_perl2");
        PERL_SET_CONTEXT(test_perl2); // Set context to perl2 before destroying it!
        perl_destruct(test_perl2);
        perl_free(test_perl2);

        cdiag("Final check on test_perl1");
        PERL_SET_CONTEXT(test_perl1); // Restore context back to perl1!
        {
            dTHX;
            SV* cached = PerlUpb_ObjCache_Get(aTHX_ dummy_ptr);
            ok(cached != NULL, "Cache still persist in perl1 after perl2 death");
            if (cached) SvREFCNT_dec(cached);
        }

        SvREFCNT_dec(strong_rv);
        SvREFCNT_dec(obj1);
    }

    perl_destruct(test_perl1);
    perl_free(test_perl1);
}

int main(int argc, char **argv, char **env) {
    PERL_SYS_INIT3(&argc, &argv, &env);
    test_multi_interp();
    PERL_SYS_TERM();
    return 0;
}
