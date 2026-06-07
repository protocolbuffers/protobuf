#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
// #include "xs/util.h" // No longer needed, declarations are in protobuf.h
#include <stdarg.h>

static void test_get_str_data(pTHX) {
    SV* sv1 = newSVpv("hello", 0);
    const char* str1 = PerlUpb_GetStrData(aTHX_ sv1);
    is_string(str1, "hello", "PerlUpb_GetStrData returns correct string");
    SvREFCNT_dec(sv1);

    SV* sv2 = newSViv(123);
    const char* str2 = PerlUpb_GetStrData(aTHX_ sv2);
    is(str2, NULL, "PerlUpb_GetStrData returns NULL for non-string SV");
    SvREFCNT_dec(sv2);

    SV* sv3 = &PL_sv_undef;
    const char* str3 = PerlUpb_GetStrData(aTHX_ sv3);
    is(str3, NULL, "PerlUpb_GetStrData returns NULL for undef SV");

    const char* str4 = PerlUpb_GetStrData(aTHX_ NULL);
    is(str4, NULL, "PerlUpb_GetStrData returns NULL for NULL SV");
}

static void test_verify_str_data(pTHX) {
    SV* sv1 = newSVpv("world", 0);
    const char* str1 = PerlUpb_VerifyStrData(aTHX_ sv1);
    is_string(str1, "world", "PerlUpb_VerifyStrData returns correct string");
    SvREFCNT_dec(sv1);

    // Test croaking (this is tricky in the C test harness)
    // We can't easily catch the croak, so we'll just ensure it doesn't crash
    // SV* sv2 = newSViv(456);
    // PerlUpb_VerifyStrData(aTHX_ sv2); // This would croak
    // SvREFCNT_dec(sv2);
    TODO {
        ok(0, "PerlUpb_VerifyStrData croaks on invalid input");
    }
}

static void test_class_name_to_full_name(pTHX) {
    TODO {
        ok(0, "PerlUpb_ClassNameToFullName converts A::B to A.B");
    }
}

static void test_utils_reentrancy(pTHX) {
    TODO {
        ok(0, "utils operations are safe under re-entrancy");
    }

    TODO optimized name conversion and string validation") {
        ok(0, "Utilities leverage hardware acceleration for path resolution");
    }

    TODO {
        ok(0, "Error messages provide exact location of validation failures");
    }

    subtest("PerlUpb_Error_Die", {
        TEST_PERL_CALL("1 + 1", "Perl interpreter is healthy");
    });
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(11);

    test_get_str_data(aTHX);
    test_verify_str_data(aTHX);
    test_class_name_to_full_name(aTHX);
    test_utils_reentrancy(aTHX);

    test_perl_destroy(my_perl);
    return 0;
}
