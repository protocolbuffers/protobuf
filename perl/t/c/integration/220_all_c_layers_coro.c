#include "t/c/upb-perl-test.h"
#include "t/c/coro_util.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/message/serialize.h"
#include "xs/message/compare.h"
#include "xs/repeated/repeated.h"
#include "xs/repeated/composite.h"
#include "xs/map/map.h"
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
#define NUM_OPS 10
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* mdef_sv;
} coro_arg_t;

void test_all_ops(pTHX_ coro_arg_t *carg) {
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ carg->mdef_sv, 0);
    const upb_MessageDef* mdef = PerlUpb_MessageDef_GetMessage(aTHX_ carg->mdef_sv);
    const upb_FieldDef* f_int32 = upb_MessageDef_FindFieldByName(mdef, "optional_int32");

    for (int i = 0; i < NUM_OPS; i++) {
        // 1. Scalar Set
        PerlUpb_Message_SetField(aTHX_ msg_sv, f_int32, newSViv(carg->id * 100 + i));

        coro_yield(carg->id); // Yield

        // 2. Serialization/Parse
        SV* serialized = PerlUpb_Message_Serialize(aTHX_ msg_sv);
        SV* parsed = PerlUpb_Message_Parse(aTHX_ carg->mdef_sv, serialized);

        if (!PerlUpb_Message_IsEqual(aTHX_ msg_sv, parsed)) {
            carg->errors++;
        }

        // 3. Unknown Fields
        SV* unk_set = PerlUpb_UnknownFieldSet_New(aTHX_ msg_sv);
        const char unk_data[] = { 0xB8, 0x3E, 0x7B }; // tag 999, val 123
        PerlUpb_UnknownFieldSet_Add(aTHX_ unk_set, newSVpvn(unk_data, sizeof(unk_data)));

        coro_yield(carg->id); // Yield

        // Cleanup iteration
        extern void PerlUpb_UnknownFieldSet_Free(pTHX_ SV* sv);
        PerlUpb_UnknownFieldSet_Free(aTHX_ unk_set);
        SvREFCNT_dec(unk_set);

        SvREFCNT_dec(serialized);
        PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ parsed));
        PerlUpb_Message_Free(aTHX_ parsed);
        SvREFCNT_dec(parsed);
    }

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    test_all_ops(aTHX_ carg);
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
        ok(0, "System handles complex message trees migrating between interpreters safely");
    }

    TODO {
        ok(0, "All C-layer components remain stable during randomized concurrent mutation");
    }

    TODO {
        ok(0, "System identifies potential lock contention in concurrent integrated usage");
    }

    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
