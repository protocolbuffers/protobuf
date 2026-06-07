#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor_pool/pool.h"

static void test_generated_pool(pTHX) {
    SV* g1 = PerlUpb_DescriptorPool_GeneratedPool(aTHX);
    ok(g1 != NULL, "GeneratedPool returns non-NULL");
    ok(sv_derived_from(g1, "Protobuf::DescriptorPool"), "GeneratedPool has correct class");

    SV* g2 = PerlUpb_DescriptorPool_GeneratedPool(aTHX);
    // Use underlying SV comparison for identity
    ok(SvRV(g1) == SvRV(g2), "GeneratedPool returns same underlying SV (singleton)");
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(10 + 2);

    // 1. Creation
    SV* pool_sv = PerlUpb_DescriptorPool_New(aTHX);
    ok(pool_sv != NULL, "PerlUpb_DescriptorPool_New returns non-NULL");
    ok(sv_derived_from(pool_sv, "Protobuf::DescriptorPool"), "Pool SV has correct class");

    // 2. Retrieval of raw pool
    const upb_DefPool* raw_pool = PerlUpb_DescriptorPool_GetPool(aTHX_ pool_sv);
    ok(raw_pool != NULL, "PerlUpb_DescriptorPool_GetPool returns raw upb_DefPool");

    // 3. Generated Pool
    test_generated_pool(aTHX);

    TODO {
        ok(0, "Read-only global pool accessible from multiple PerlInterpreter instances");
    }

    TODO {
        ok(0, "Multiple interpreters share a single optimized memory segment for descriptors");
    }

    TODO {
        ok(0, "Descriptor startup time is proportional to used definitions, not total schema size");
    }

    TODO {
        ok(0, "Diagnostic errors identify the exact source location of descriptor naming conflicts");
    }

    TODO {
        ok(0, "Add cross-process fingerprinting for pool identity");
        ok(0, "Implement sub-second startup benchmarks for large pools");
    }

    // Cleanup
    SvREFCNT_dec(pool_sv);

    test_perl_destroy(my_perl);
    return 0;
}
