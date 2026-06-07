#include "t/c/upb-perl-test.h"
#include "xs/protobuf/obj_cache.h"
#include <time.h>
#include <stdlib.h>

void run_bench(PerlInterpreter* original_perl, int n) {
    dTHX;
    PERL_SET_CONTEXT(original_perl);
    void** ptrs = malloc(sizeof(void*) * n);
    SV** rvs = malloc(sizeof(SV*) * n);

    for (int i = 0; i < n; i++) {
        ptrs[i] = (void*)(uintptr_t)(i + 1);
        rvs[i] = newRV_noinc(newSViv(i));
    }

    // Benchmark Add
    clock_t start = clock();
    for (int i = 0; i < n; i++) {
        PerlUpb_ObjCache_Add(aTHX_ ptrs[i], rvs[i]);
    }
    clock_t end = clock();
    double add_time = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark Get
    start = clock();
    for (int i = 0; i < n; i++) {
        SV* got = PerlUpb_ObjCache_Get(aTHX_ ptrs[i]);
        if (!got) {
            fprintf(stderr, "Failed to get ptr %d\n", i);
            exit(1);
        }
        SvREFCNT_dec(got);
    }
    end = clock();
    double get_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Scale: %d | Add: %.4f s (%.2f ops/sec) | Get: %.4f s (%.2f ops/sec)\n",
           n, add_time, n / add_time, get_time, n / get_time);

    PerlUpb_ObjCache_Clear(aTHX);
    for (int i = 0; i < n; i++) {
        SvREFCNT_dec(rvs[i]);
    }
    free(ptrs);
    free(rvs);
}

int main(int argc, char** argv) {
    PERL_SYS_INIT(&argc, &argv);
    PerlInterpreter *my_perl = test_perl_init(argc, argv);
    {
        dTHX;
        PerlUpb_ObjCache_Init(aTHX);

        printf("Object Cache Benchmarks:\n");
        run_bench(my_perl, 1000);
        run_bench(my_perl, 10000);
        run_bench(my_perl, 100000);
        // 1M might be slow due to weak ref management, let's see
        run_bench(my_perl, 1000000);
    }
    test_perl_destroy(my_perl);
    PERL_SYS_TERM();
    return 0;
}
