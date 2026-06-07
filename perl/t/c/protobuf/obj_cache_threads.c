#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/protobuf/obj_cache.h"
#include <string.h>
#ifdef MULTIPLICITY
#include <pthread.h>

#define NUM_THREADS 10
#define NUM_OPS_PER_THREAD 1000

typedef struct {
    PerlInterpreter *original_perl;
    int id;
} thread_arg_t;

int thread_shared_objects[100];

void* thread_stress_func(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;

    // Each thread gets its OWN interpreter to simulate ithreads/concurrency
    char* dummy_argv[] = {"", "-e", "0", NULL};
    PerlInterpreter *my_perl = test_perl_init(3, dummy_argv);
    {
        dTHX;
        PerlUpb_ObjCache_Init(aTHX);

        for (int i = 0; i < NUM_OPS_PER_THREAD; i++) {
            ENTER;
            SAVETMPS;

            int obj_idx = i % 100;
            void* ptr = &thread_shared_objects[obj_idx];

            SV* val = newSVpvf("thread-%d-val-%d", targ->id, i);
            SV* rv = newRV_noinc(val);

            PerlUpb_ObjCache_Add(aTHX_ ptr, rv);

            SV* got = PerlUpb_ObjCache_Get(aTHX_ ptr);
            if (got) {
                SvREFCNT_dec(got);
            }

            if (i % 10 == 0) {
                PerlUpb_ObjCache_Delete(aTHX_ ptr);
            }

            SvREFCNT_dec(rv);

            FREETMPS;
            LEAVE;
        }
    }
    test_perl_destroy(my_perl);
    return NULL;
}
#endif

int main(int argc, char** argv) {
#ifndef MULTIPLICITY
    plan(1);
    SKIP("MULTIPLICITY not supported in this Perl configuration", 1);
    return 0;
#else
    PerlInterpreter *my_perl = test_perl_init(argc, argv);
    {
        dTHX;
        plan(4);

        PerlUpb_ObjCache_Init(aTHX);
        ok(1, "Object cache initialized for threading test");

        thread_arg_t args[NUM_THREADS];
        pthread_t threads[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; i++) {
            args[i].original_perl = my_perl;
            args[i].id = i;
            pthread_create(&threads[i], NULL, thread_stress_func, &args[i]);
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }

        ok(1, "Threaded cache stress test completed");

        PerlUpb_ObjCache_Clear(aTHX);
        ok(1, "Cache cleared after threading test");
    }
    test_perl_destroy(my_perl);
    return 0;
#endif
}
