#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/repeated/repeated.h"
#include "xs/repeated/composite.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "libcoro/coro.h"
#include "t/c/coro_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define NUM_COROS 10
#define NUM_OPS 10
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* mdef_sv;
} coro_arg_t;

void test_repeated_ops(pTHX_ coro_arg_t *carg) {
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ carg->mdef_sv, 0);
    const upb_MessageDef* mdef = PerlUpb_MessageDef_GetMessage(aTHX_ carg->mdef_sv);
    const upb_FieldDef* f_rep_int32 = upb_MessageDef_FindFieldByName(mdef, "repeated_int32");

    for (int i = 0; i < NUM_OPS; i++) {
        AV* av = newAV();
        for (int j = 0; j < 5; j++) {
            av_push(av, newSViv(carg->id * 100 + i * 10 + j));
        }
        SV* av_ref = newRV_noinc((SV*)av);
        PerlUpb_Message_SetField(aTHX_ msg_sv, f_rep_int32, av_ref);

        coro_yield(carg->id);

        SV* ret_av_ref = PerlUpb_Message_GetField(aTHX_ msg_sv, f_rep_int32);
        if (PerlUpb_Repeated_Size(aTHX_ ret_av_ref) != 5) {
            carg->errors++;
        }
        SvREFCNT_dec(ret_av_ref);
        SvREFCNT_dec(av_ref);
    }

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    test_repeated_ops(aTHX_ carg);
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
        ok(0, "System remains stable when multiple coroutines force array reallocation");
    }

    TODO {
        ok(0, "Sub-message array wrappers consistently follow ObjCache rules across coroutines");
    }

    TODO {
        ok(0, "TSan-equivalent checks for concurrent repeated field accessor usage");
    }

    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
