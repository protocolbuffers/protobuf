#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/message.h"
#include "xs/descriptor/field.h"
#include "xs/protobuf/arena.h"
#include "t/c/convert/test_util.h"
#include "libcoro/coro.h"
#include "t/c/coro_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define NUM_COROS 50
#define NUM_OPS 100
#define STACK_SIZE 32768

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
} coro_arg_t;

void test_descriptor_access(pTHX_ coro_arg_t *carg) {
    const upb_MessageDef *msg_def = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    if (!msg_def) {
        fprintf(stderr, "Coro %d: Failed to find protobuf_perl_test.TestMessage\n", carg->id);
        carg->errors++;
        return;
    }

    const char* full_name = upb_MessageDef_FullName(msg_def);
    if (strcmp(full_name, "protobuf_perl_test.TestMessage") != 0) {
        fprintf(stderr, "Coro %d: MessageFullName mismatch\n", carg->id);
        carg->errors++;
    }

    coro_yield(carg->id);

    const upb_FieldDef *field = upb_MessageDef_FindFieldByName(msg_def, "value");
    if (!field) {
        fprintf(stderr, "Coro %d: Failed to find value\n", carg->id);
        carg->errors++;
        return;
    }

    if (upb_FieldDef_Type(field) != kUpb_FieldType_Int32) {
        fprintf(stderr, "Coro %d: FieldType mismatch\n", carg->id);
        carg->errors++;
    }
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX; // Declare my_perl

    for (int i = 0; i < NUM_OPS; i++) {
        test_descriptor_access(aTHX_ carg);
        coro_yield(carg->id);
    }
    coro_finish(carg->id);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(2 + NUM_COROS + 3);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) {
         fprintf(stderr, "Failed to load test descriptors\n");
         return 1;
    }
    ok(1, "Descriptors loaded");

    coro_arg_t args[NUM_COROS];
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "System remains stable when multiple coroutines resolve MessageSubDef concurrently");
    }

    TODO {
        ok(0, "Descriptor wrapping and cache lookup are safe across interleaved coroutines");
    }

    TODO {
        ok(0, "Pool reference counting remains robust under high concurrency");
    }

    PerlUpb_Arena_Destroy(aTHX_ arena_sv);
    test_perl_destroy(my_perl);
    return 0;
}
