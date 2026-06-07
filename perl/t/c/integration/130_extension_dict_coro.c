#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/extension_dict/dict.h"
#include "xs/extension_dict/iterator.h"
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

#define NUM_COROS 20
#define NUM_OPS 50
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* dict_sv;
    SV* field_sv;
} coro_arg_t;

void test_extension_access(pTHX_ coro_arg_t *carg) {
    char val_str[32];
    sprintf(val_str, "coro %d op", carg->id);
    SV* val_sv = newSVpv(val_str, 0);

    // Set value
    PerlUpb_ExtensionDict_SetItem(aTHX_ carg->dict_sv, carg->field_sv, val_sv);

    // Yield immediately after set to let others overwrite
    coro_yield(carg->id);

    // Get value. Note: in this specific test, multiple coros are fighting
    // for the SAME field, so we might not get our own value back
    // if another coro ran in between.
    // However, since we yield to main and then main transfers to the NEXT coro,
    // we CAN predict the behavior.

    SV* ret_val = PerlUpb_ExtensionDict_GetItem(aTHX_ carg->dict_sv, carg->field_sv);
    // (We don't assert the exact value here because of the contention,
    // just that it's a valid string)
    if (!SvPOK(ret_val)) {
        carg->errors++;
    }

    SvREFCNT_dec(ret_val);
    SvREFCNT_dec(val_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    for (int i = 0; i < NUM_OPS; i++) {
        test_extension_access(aTHX_ carg);
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

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    const upb_FieldDef *ext_field = upb_DefPool_FindExtensionByName(test_pool, "protobuf_perl_test.extension_string");

    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);
    SV* message_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
    SV* dict_sv = PerlUpb_ExtensionDict_New(aTHX_ message_sv);
    SV* field_sv = PerlUpb_FieldDef_GetWrapper(aTHX_ ext_field);

    coro_arg_t args[NUM_COROS];
    for (int i = 0; i < NUM_COROS; i++) {
        args[i].dict_sv = dict_sv;
        args[i].field_sv = field_sv;
    }
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "Multiple coroutines modifying different extensions in one message verified");
    }

    TODO {
        ok(0, "Extension field wrappers are consistently cached across coroutines");
    }

    TODO {
        ok(0, "System remains stable under massive concurrent iteration of extensions");
    }

    SvREFCNT_dec(field_sv);
    extern void PerlUpb_ExtensionDict_Free(pTHX_ SV* sv);
    PerlUpb_ExtensionDict_Free(aTHX_ dict_sv);
    SvREFCNT_dec(dict_sv);
    SvREFCNT_dec(message_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
