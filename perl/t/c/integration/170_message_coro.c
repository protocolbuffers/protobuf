#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/message/serialize.h"
#include "xs/message/compare.h"
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
#define NUM_OPS 20
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* mdef_sv;
} coro_arg_t;

void test_message_ops(pTHX_ coro_arg_t *carg) {
    // 1. Create message
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ carg->mdef_sv, 0);
    if (!msg_sv || !sv_isobject(msg_sv)) {
        carg->errors++;
        return;
    }

    const upb_MessageDef* mdef = PerlUpb_MessageDef_GetMessage(aTHX_ carg->mdef_sv);
    const upb_FieldDef* f_int32 = upb_MessageDef_FindFieldByName(mdef, "optional_int32");

    // 2. Set/Get
    char buf[32];
    sprintf(buf, "val %d", carg->id);
    SV* val_sv = newSViv(carg->id * 1000);
    PerlUpb_Message_SetField(aTHX_ msg_sv, f_int32, val_sv);

    coro_yield(carg->id);

    SV* ret_val = PerlUpb_Message_GetField(aTHX_ msg_sv, f_int32);
    if (SvIV(ret_val) != (carg->id * 1000)) {
        carg->errors++;
    }
    SvREFCNT_dec(ret_val);

    // 3. Serialize/Parse
    SV* serialized = PerlUpb_Message_Serialize(aTHX_ msg_sv);
    coro_yield(carg->id);

    SV* parsed_sv = PerlUpb_Message_Parse(aTHX_ carg->mdef_sv, serialized);
    if (!parsed_sv || !PerlUpb_Message_IsEqual(aTHX_ msg_sv, parsed_sv)) {
        carg->errors++;
    }

    // Cleanup
    SvREFCNT_dec(val_sv);
    SvREFCNT_dec(serialized);

    if (msg_sv) {
        PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
        PerlUpb_Message_Free(aTHX_ msg_sv);
        SvREFCNT_dec(msg_sv);
    }

    if (parsed_sv) {
        PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ parsed_sv));
        PerlUpb_Message_Free(aTHX_ parsed_sv);
        SvREFCNT_dec(parsed_sv);
    }
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    for (int i = 0; i < NUM_OPS; i++) {
        test_message_ops(aTHX_ carg);
        coro_yield(carg->id);
    }
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
        ok(0, "System remains stable during high-frequency allocation/deallocation of coroutine messages");
    }

    TODO {
        ok(0, "Message wrappers consistently follow ObjCache rules across coroutines");
    }

    TODO {
        ok(0, "TSan-equivalent checks for concurrent message accessor usage");
    }

    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
