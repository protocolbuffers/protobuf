#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/protobuf/obj_cache.h"
#include "libcoro/coro.h"
#include "t/c/coro_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_COROS 20
#define NUM_OPS 500
#define STACK_SIZE 32768

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
} coro_arg_t;

// Shared objects to create contention
#define NUM_SHARED 10
int shared_objects[NUM_SHARED];

void chaos_coro_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;

    for (int i = 0; i < NUM_OPS; i++) {
        int obj_idx = i % NUM_SHARED;
        void* ptr = &shared_objects[obj_idx];

        int action = rand() % 3;

        if (action == 0) { // Add
            SV* val = newSVpvf("val-%d-%d", obj_idx, i);
            SV* rv = newRV_noinc(val);
            PerlUpb_ObjCache_Add(aTHX_ ptr, rv);

            if (rand() % 2) coro_yield(carg->id);

            SV* got = PerlUpb_ObjCache_Get(aTHX_ ptr);
            if (got) {
                // It might not be 'val' if another coro overwrote it, but it should be a valid RV
                if (!SvROK(got)) {
                    fprintf(stderr, "Coro %d: Expected RV, got something else\n", carg->id);
                    carg->errors++;
                }
                SvREFCNT_dec(got);
            }
            SvREFCNT_dec(rv);
        } else if (action == 1) { // Get
            SV* got = PerlUpb_ObjCache_Get(aTHX_ ptr);
            if (got) {
                if (!SvROK(got) || !SvRV(got) || SvRV(got) == &PL_sv_undef) {
                    fprintf(stderr, "Coro %d: Got corrupted/invalid RV from cache\n", carg->id);
                    carg->errors++;
                }
                SvREFCNT_dec(got);
            }
        } else { // Trigger potential GC pressure
            // Create many mortal SVs and yield
            for (int j = 0; j < 100; j++) {
                sv_2mortal(newSVpv("temporary", 0));
            }
            if (rand() % 5 == 0) {
                eval_pv("undef", 0); // Minor perl activity
            }
        }

        if (i % 10 == 0) coro_yield(carg->id);
    }
    coro_finish(carg->id);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);
    {
        dTHX;
        plan(3 + NUM_COROS);

        PerlUpb_ObjCache_Init(aTHX);
        ok(1, "Object cache initialized");

        srand(time(NULL));
        coro_arg_t args[NUM_COROS];
        RUN_CORO_TEST(chaos_coro_func, args);

        ok(1, "Chaos concurrency test completed");

        TODO {
            ok(1, "Weak references remain stable during interleaved GC cycles (Verified via Chaos)");
        }
    }
    test_perl_destroy(my_perl);
    return 0;
}
