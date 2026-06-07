#include "t/c/upb-perl-test.h"
#include "t/c/coro_util.h"
#include "xs/protobuf.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/message/serialize.h"
#include "xs/unknown_fields/set.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define NUM_COROS 10
#define NUM_OPS 20
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* mdef_sv;
} coro_arg_t;

void test_unknown_fields_ops(pTHX_ coro_arg_t *carg) {
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ carg->mdef_sv, 0);
    SV* set_sv = PerlUpb_UnknownFieldSet_New(aTHX_ msg_sv);

    for (int i = 0; i < NUM_OPS; i++) {
        char buf[16];
        sprintf(buf, "unk%d_%d", carg->id, i);
        SV* data_sv = newSVpv(buf, 0);
        PerlUpb_UnknownFieldSet_Add(aTHX_ set_sv, data_sv);
        SvREFCNT_dec(data_sv);

        coro_yield(carg->id);

        SV* ret_data = PerlUpb_UnknownFieldSet_GetData(aTHX_ set_sv);
        if (SvCUR(ret_data) == 0) {
            carg->errors++;
        }
        SvREFCNT_dec(ret_data);
    }

    PerlUpb_UnknownFieldSet_Free(aTHX_ set_sv);
    SvREFCNT_dec(set_sv);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    test_unknown_fields_ops(aTHX_ carg);
    coro_finish(carg->id);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(2 + NUM_COROS + 3);

    extern void PerlUpb_ObjCache_Init(pTHX);
    PerlUpb_ObjCache_Init(aTHX);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    if (!load_test_descriptors(aTHX_ arena)) return 1;
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    SV* mdef_sv = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);

    coro_arg_t args[NUM_COROS];
    for (int i = 0; i < NUM_COROS; i++) {
        args[i].mdef_sv = mdef_sv;
    }
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "System remains stable when multiple coroutines force unknown buffer reallocation");
    }

    TODO {
        ok(0, "Message wrappers created from unknown blobs consistently follow ObjCache rules");
    }

    TODO {
        ok(0, "TSan-equivalent checks for concurrent unknown data access");
    }

    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
