#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/map/map.h"
#include "xs/map/iterator.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "libcoro/coro.h"
#include "t/c/coro_util.h"
#include "t/c/convert/test_util.h"
#include "upb/reflection/message.h"
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
    SV* map_sv;
} coro_arg_t;

void test_map_access(pTHX_ coro_arg_t *carg) {
    SV* key_sv = newSViv(carg->id);
    SV* val_sv = newSViv(carg->id * 100);

    PerlUpb_Map_SetItem(aTHX_ carg->map_sv, key_sv, val_sv);
    coro_yield(carg->id);

    SV* ret_val = PerlUpb_Map_GetItem(aTHX_ carg->map_sv, key_sv);
    if (!SvIOK(ret_val) || SvIV(ret_val) != (carg->id * 100)) {
        fprintf(stderr, "Coro %d: Map value mismatch\n", carg->id);
        carg->errors++;
    }
    SvREFCNT_dec(ret_val);
    SvREFCNT_dec(key_sv);
    SvREFCNT_dec(val_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    for (int i = 0; i < NUM_OPS; i++) {
        test_map_access(aTHX_ carg);
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
    const upb_FieldDef *map_field = upb_MessageDef_FindFieldByName(mdef, "map_int32_int32");
    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);
    upb_Map* map_ptr = upb_Message_Mutable(msg, map_field, arena).map;
    SV* map_sv = PerlUpb_Map_New(aTHX_ map_ptr, map_field, arena_sv, 0);

    coro_arg_t args[NUM_COROS];
    for (int i = 0; i < NUM_COROS; i++) {
        args[i].map_sv = map_sv;
    }
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "System remains stable when one coroutine iterates while others modify the map");
    }

    TODO {
        ok(0, "Map value wrappers consistently follow ObjCache rules across coroutines");
    }

    TODO {
        ok(0, "Arena expansion and map resizing are thread-safe in the C layer");
    }

    SvREFCNT_dec(map_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
