#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor_containers/by_name_map.h"
#include "xs/descriptor_containers/generic_sequence.h"
#include "xs/descriptor_containers/iterators.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "t/c/convert/test_util.h"
#include "upb/reflection/def.h"
#include <stdio.h>

// VTable for MessageDef.fields (Sequence)
int msg_field_count(const void* p) { return upb_MessageDef_FieldCount((const upb_MessageDef*)p); }
const void* msg_field_get(const void* p, int i) { return upb_MessageDef_Field((const upb_MessageDef*)p, i); }
SV* msg_field_wrap(pTHX_ const void* p) {
    SV* sv = newSViv((IV)p);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::Descriptor::Field", GV_ADD));
    return obj;
}

static const PerlUpb_GenericSequence_VTable msg_fields_seq_vtable = {
    msg_field_count, msg_field_get, msg_field_wrap
};

// VTable for MessageDef.fields_by_name (ByNameMap)
const void* msg_field_lookup(const void* p, const char* n) { return upb_MessageDef_FindFieldByName((const upb_MessageDef*)p, n); }
const char* msg_field_key(const void* p, int i) {
    const upb_FieldDef* f = upb_MessageDef_Field((const upb_MessageDef*)p, i);
    return f ? upb_FieldDef_Name(f) : NULL;
}
const void* msg_field_value(const void* p, int i) { return upb_MessageDef_Field((const upb_MessageDef*)p, i); }

static const PerlUpb_ByNameMap_VTable msg_fields_map_vtable = {
    msg_field_count, msg_field_lookup, msg_field_key, msg_field_value, msg_field_wrap
};

static void test_map_iterator(pTHX_ SV* map_sv) {
    SV* iter_sv = PerlUpb_DescriptorMapIterator_New(aTHX_ map_sv);
    ok(iter_sv != NULL, "Created MapIterator");
    ok(sv_derived_from(iter_sv, "Protobuf::Internals::DescriptorMapIterator"), "Blessed correctly");

    int total = PerlUpb_ByNameMap_Count(aTHX_ map_sv);
    int count = 0;
    while (1) {
        SV* key = PerlUpb_DescriptorMapIterator_NextKey(aTHX_ iter_sv);
        if (key == &PL_sv_undef) break;

        SV* val = PerlUpb_DescriptorMapIterator_NextValue(aTHX_ iter_sv);
        // Only check first few to avoid flooding TAP output
        if (count < 2) {
            ok(SvPOK(key), "Iterator key is a string");
            ok(sv_derived_from(val, "Protobuf::Descriptor::Field"), "Iterator value is a FieldDescriptor");
        }

        SvREFCNT_dec(key);
        SvREFCNT_dec(val);
        count++;
    }

    is(count, total, "Iterated correct number of items");

    extern void PerlUpb_DescriptorMapIterator_Free(pTHX_ SV* sv);
    PerlUpb_DescriptorMapIterator_Free(aTHX_ iter_sv);
    SvREFCNT_dec(iter_sv);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(20);

    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
         fail("Failed to load test descriptors");
         return 1;
    }
    ok(1, "Descriptors loaded");

    const upb_MessageDef *msg_def = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    ok(msg_def != NULL, "Found protobuf_perl_test.TestMessage");

    if (msg_def) {
        SV* parent_sv = newSViv(1); // Fake parent

        // 1. Test Sequence (fields)
        SV* seq_sv = PerlUpb_GenericSequence_New(aTHX_ parent_sv, msg_def, &msg_fields_seq_vtable);
        ok(seq_sv != NULL, "Created fields sequence");
        int count = PerlUpb_GenericSequence_Count(aTHX_ seq_sv);
        ok(count > 0, "Field count > 0");

        SV* f0 = PerlUpb_GenericSequence_GetItem(aTHX_ seq_sv, 0);
        ok(sv_derived_from(f0, "Protobuf::Descriptor::Field"), "Item 0 is a FieldDescriptor");
        SvREFCNT_dec(f0);

        // 2. Test ByNameMap (fields_by_name)
        SV* map_sv = PerlUpb_ByNameMap_New(aTHX_ parent_sv, msg_def, &msg_fields_map_vtable);
        ok(map_sv != NULL, "Created fields_by_name map");
        is(PerlUpb_ByNameMap_Count(aTHX_ map_sv), count, "Map count matches sequence count");

        SV* val_sv = PerlUpb_ByNameMap_Lookup(aTHX_ map_sv, "value");
        ok(sv_derived_from(val_sv, "Protobuf::Descriptor::Field"), "Lookup 'value' returned a FieldDescriptor");

        const upb_FieldDef* f_raw = (const upb_FieldDef*)SvIV(SvRV(val_sv));
        is_string(upb_FieldDef_Name(f_raw), "value", "Raw field name is 'value'");

        SvREFCNT_dec(val_sv);

        SV* key0 = PerlUpb_ByNameMap_Key(aTHX_ map_sv, 0);
        ok(SvPOK(key0), "Key 0 is a string");
        SvREFCNT_dec(key0);

        SV* val0 = PerlUpb_ByNameMap_Value(aTHX_ map_sv, 0);
        ok(sv_derived_from(val0, "Protobuf::Descriptor::Field"), "Value 0 is a FieldDescriptor");
        SvREFCNT_dec(val0);

        test_map_iterator(aTHX_ map_sv);

        TODO {
            ok(0, "Enum values correctly integrated with ByNumberMap container logic");
        }

        TODO") {
            ok(0, "Entire descriptor sequence projected into native Perl AV in one pass");
        }

        TODO {
            ok(0, "Containers remain stable under concurrent access from independent interpreters");
        }

        TODO {
            ok(0, "Iterators safely detect and report when their source container is invalid");
        }

        extern void PerlUpb_ByNameMap_Free(pTHX_ SV* sv);
        PerlUpb_ByNameMap_Free(aTHX_ map_sv);
        extern void PerlUpb_GenericSequence_Free(pTHX_ SV* sv);
        PerlUpb_GenericSequence_Free(aTHX_ seq_sv);

        SvREFCNT_dec(seq_sv);
        SvREFCNT_dec(map_sv);
        SvREFCNT_dec(parent_sv);
    }

    upb_Arena_Free(arena);
    test_perl_destroy(my_perl);
    return 0;
}
