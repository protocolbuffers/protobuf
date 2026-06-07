#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/map/map.h"
#include "xs/map/iterator.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "upb/message/map.h"
#include "upb/reflection/message.h"
#include <stdio.h>

static void test_map_as_hash(pTHX_ SV* map_sv) {
    SV* hash_rv = PerlUpb_Map_AsHash(aTHX_ map_sv);
    ok(SvROK(hash_rv) && SvTYPE(SvRV(hash_rv)) == SVt_PVHV, "AsHash returns hash ref");
    HV* hv = (HV*)SvRV(hash_rv);

    is(hv_iterinit(hv), 1, "Projected map hash has 1 key");

    SV** val_ptr = hv_fetch(hv, "10", 2, 0);
    ok(val_ptr != NULL, "Found key '10' in projected hash");
    if (val_ptr) {
        is(SvIV(*val_ptr), 42, "Value for key '10' is 42");
    }
    SvREFCNT_dec(hash_rv);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(17);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) {
         fail("Failed to load test descriptors");
         return 1;
    }
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);

    // 1. Test map<int32, int32>
    const upb_FieldDef *map_ii_field = upb_MessageDef_FindFieldByName(mdef, "map_int32_int32");
    ok(map_ii_field != NULL, "Found map_int32_int32");

    upb_Map* map_ii = upb_Message_Mutable(msg, map_ii_field, arena).map;
    SV* map_ii_sv = PerlUpb_Map_New(aTHX_ map_ii, map_ii_field, arena_sv, 0);

    SV* key_ii = newSViv(10);
    SV* val_ii = newSViv(42);
    PerlUpb_Map_SetItem(aTHX_ map_ii_sv, key_ii, val_ii);

    SV* ret_ii = PerlUpb_Map_GetItem(aTHX_ map_ii_sv, key_ii);
    is(SvIV(ret_ii), 42, "int32->int32 map roundtrip");
    SvREFCNT_dec(ret_ii);

    // 2. Test map<string, string>
    const upb_FieldDef *map_ss_field = upb_MessageDef_FindFieldByName(mdef, "map_string_string");
    ok(map_ss_field != NULL, "Found map_string_string");
    upb_Map* map_ss = upb_Message_Mutable(msg, map_ss_field, arena).map;
    SV* map_ss_sv = PerlUpb_Map_New(aTHX_ map_ss, map_ss_field, arena_sv, 0);

    SV* key_ss = newSVpv("key2", 0);
    SV* val_ss = newSVpv("value2", 0);

    PerlUpb_Map_SetItem(aTHX_ map_ss_sv, key_ss, val_ss);

    SV* ret_ss = PerlUpb_Map_GetItem(aTHX_ map_ss_sv, key_ss);
    is_string(SvPV_nolen(ret_ss), "value2", "string->string map roundtrip");

    SvREFCNT_dec(ret_ss);
    SvREFCNT_dec(val_ss);
    SvREFCNT_dec(map_ss_sv);

    // 3. Test Iterator for map<int32, int32>
    SV* iter_sv = PerlUpb_Map_GetIterator(aTHX_ map_ii_sv);
    SV* k = PerlUpb_Map_Iterator_NextKey(aTHX_ iter_sv);
    is(SvIV(k), 10, "Iterator key matches");
    SV* v = PerlUpb_Map_Iterator_Value(aTHX_ iter_sv);
    is(SvIV(v), 42, "Iterator value matches");

    SvREFCNT_dec(k);
    SvREFCNT_dec(v);
    SvREFCNT_dec(iter_sv);

    test_map_as_hash(aTHX_ map_ii_sv);

    TODO {
        ok(0, "Sub-messages retrieved from maps are correctly cached and identical");
    }

    TODO {
        ok(0, "Iterators and getters remain safe while map entries are removed");
    }

    TODO {
        ok(0, "Perl wrappers for map elements only created on-demand");
    }

    TODO {
        ok(0, "Deep-copy logic for maps between different arenas verified");
    }

    TODO") {
        ok(0, "Bulk validation of map keys achieves line-rate throughput");
    }

    // Cleanup
    SvREFCNT_dec(map_ii_sv);

    SvREFCNT_dec(key_ii);
    SvREFCNT_dec(val_ii);
    SvREFCNT_dec(key_ss);

    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
