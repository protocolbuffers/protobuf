#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor_containers/by_name_map.h"
#include "xs/descriptor_containers/generic_sequence.h"
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
    const upb_MessageDef *msg_def;
    SV* parent_sv;
} coro_arg_t;

// Re-use vtables from 090_descriptor_containers.c (could be shared in a real project)
int msg_field_count(const void* p) { return upb_MessageDef_FieldCount((const upb_MessageDef*)p); }
const void* msg_field_get(const void* p, int i) { return upb_MessageDef_Field((const upb_MessageDef*)p, i); }
SV* msg_field_wrap(pTHX_ const void* p) { return newSViv((IV)p); }
static const PerlUpb_GenericSequence_VTable msg_fields_seq_vtable = { msg_field_count, msg_field_get, msg_field_wrap };

const void* msg_field_lookup(const void* p, const char* n) { return upb_MessageDef_FindFieldByName((const upb_MessageDef*)p, n); }
const char* msg_field_key(const void* p, int i) {
    const upb_FieldDef* f = upb_MessageDef_Field((const upb_MessageDef*)p, i);
    return f ? upb_FieldDef_Name(f) : NULL;
}
const void* msg_field_value(const void* p, int i) { return upb_MessageDef_Field((const upb_MessageDef*)p, i); }
static const PerlUpb_ByNameMap_VTable msg_fields_map_vtable = { msg_field_count, msg_field_lookup, msg_field_key, msg_field_value, msg_field_wrap };

void test_container_access(pTHX_ coro_arg_t *carg) {
    // Test map creation and lookup
    SV* map_sv = PerlUpb_ByNameMap_New(aTHX_ carg->parent_sv, carg->msg_def, &msg_fields_map_vtable);
    if (!map_sv) { carg->errors++; return; }

    SV* val_sv = PerlUpb_ByNameMap_Lookup(aTHX_ map_sv, "value");
    if (!SvOK(val_sv)) {
        fprintf(stderr, "Coro %d: Failed to find 'value'\n", carg->id);
        carg->errors++;
    }
    SvREFCNT_dec(val_sv);

    coro_yield(carg->id);

    // Test sequence creation and count
    SV* seq_sv = PerlUpb_GenericSequence_New(aTHX_ carg->parent_sv, carg->msg_def, &msg_fields_seq_vtable);
    if (!seq_sv) { carg->errors++; return; }

    if (PerlUpb_GenericSequence_Count(aTHX_ seq_sv) <= 0) {
        fprintf(stderr, "Coro %d: Field count <= 0\n", carg->id);
        carg->errors++;
    }

    // Explicitly free the map and sequence internal state for the test (normally DESTROY handles this)
    extern void PerlUpb_ByNameMap_Free(pTHX_ SV* sv);
    PerlUpb_ByNameMap_Free(aTHX_ map_sv);
    extern void PerlUpb_GenericSequence_Free(pTHX_ SV* sv);
    PerlUpb_GenericSequence_Free(aTHX_ seq_sv);

    SvREFCNT_dec(map_sv);
    SvREFCNT_dec(seq_sv);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    for (int i = 0; i < NUM_OPS; i++) {
        test_container_access(aTHX_ carg);
        coro_yield(carg->id);
    }
    coro_finish(carg->id);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(2 + NUM_COROS + 2);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    if (!load_test_descriptors(aTHX_ arena)) { return 1; }
    ok(1, "Descriptors loaded");

    const upb_MessageDef *msg_def = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    if (!msg_def) { return 1; }

    SV* parent_sv = newSViv(1);
    coro_arg_t args[NUM_COROS];
    for (int i = 0; i < NUM_COROS; i++) {
        args[i].msg_def = msg_def;
        args[i].parent_sv = parent_sv;
    }
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "System remains stable under massive creation/destruction of short-lived container wrappers");
    }

    TODO {
        ok(0, "Container lookups consistently return cached descriptor instances across coroutines");
    }

    SvREFCNT_dec(parent_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);
    test_perl_destroy(my_perl);
    return 0;
}
