#include "t/c/upb-perl-test.h"
#include "xs/convert/sv_to_upb.h"
#include "xs/convert/upb_to_sv.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/registry.h"
#include "xs/descriptor_pool/find.h"
#include "xs/descriptor_pool/pool.h"
#include "t/c/convert/test_util.h"
#include "libcoro/coro.h"

#define NUM_COROS 10
#define OPS_PER_CORO 50

typedef struct {
    int id;
    PerlInterpreter *my_perl;
    coro_context *main_ctx;
    coro_context *my_ctx;
    bool *finished;
    upb_Arena *arena;
    const upb_FieldDef *f;
} coro_arg_t;

static void convert_coro_task(void *arg) {
    coro_arg_t *carg = (coro_arg_t*)arg;
    pTHX = carg->my_perl;

    for (int i = 0; i < OPS_PER_CORO; i++) {
        SV* sv = newSViv(i);
        upb_MessageValue val;

        if (PerlUpb_SvToUpb(aTHX_ sv, carg->f, &val, carg->arena)) {
            SV* back = PerlUpb_UpbToSv(aTHX_ &val, carg->f, NULL);
            if (SvIV(back) != i) {
                fprintf(stderr, "Conversion mismatch in coro %d: expected %d, got %ld\n", carg->id, i, (long)SvIV(back));
            }
            SvREFCNT_dec(back);
        }
        SvREFCNT_dec(sv);

        if (i % 5 == 0) {
            coro_transfer(carg->my_ctx, carg->main_ctx);
        }
    }

    *carg->finished = true;
    coro_transfer(carg->my_ctx, carg->main_ctx);
}

static void test_convert_coro(void) {
    dTHX;
    plan(NUM_COROS);

    coro_context main_ctx;
    coro_context ctxs[NUM_COROS];
    bool finished[NUM_COROS];
    char stacks[NUM_COROS][65536];
    coro_arg_t args[NUM_COROS];

    coro_create(&main_ctx, NULL, NULL, NULL, 0);

    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
        fprintf(stderr, "Failed to load test descriptors\n");
        return;
    }

    SV* pool_sv = PerlUpb_DescriptorPool_GeneratedPool(aTHX);
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ pool_sv);
    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(pool, "protobuf_perl_test.TestMessage");
    if (!mdef) {
        cdiag("Could not find message protobuf_perl_test.TestMessage");
        upb_Arena_Free(arena);
        return;
    }
    const upb_FieldDef *f = upb_MessageDef_FindFieldByNumber(mdef, 1);

    for (int i = 0; i < NUM_COROS; i++) {
        finished[i] = false;
        args[i].id = i;
        args[i].my_perl = aTHX;
        args[i].main_ctx = &main_ctx;
        args[i].my_ctx = &ctxs[i];
        args[i].finished = &finished[i];
        args[i].arena = arena;
        args[i].f = f;

        coro_create(&ctxs[i], convert_coro_task, &args[i], stacks[i], sizeof(stacks[i]));
    }

    int finished_count = 0;
    while (finished_count < NUM_COROS) {
        finished_count = 0;
        for (int i = 0; i < NUM_COROS; i++) {
            if (!finished[i]) {
                coro_transfer(&main_ctx, &ctxs[i]);
            } else {
                finished_count++;
            }
        }
    }

    upb_Arena_Free(arena);

    for (int i = 0; i < NUM_COROS; i++) {
        ok(1, "Coroutine completed");
    }
}

int main(int argc, char **argv, char **env) {
    PERL_SYS_INIT3(&argc, &argv, &env);

    PerlInterpreter *test_perl = perl_alloc();
    perl_construct(test_perl);
    char *my_argv[] = { "", "-e", "0", NULL };
    perl_parse(test_perl, NULL, 3, my_argv, NULL);
    perl_run(test_perl);
    PERL_SET_CONTEXT(test_perl);
    {
        dTHX;
        PerlUpb_Registry_Init(aTHX);
        PerlUpb_ObjCache_Init(aTHX);
        test_convert_coro();
    }
    perl_destruct(test_perl);
    perl_free(test_perl);

    PERL_SYS_TERM();
    return 0;
}
