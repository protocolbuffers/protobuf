#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/descriptor_containers/iterators.h"
#include "xs/descriptor_containers/by_name_map.h"
#include <stdio.h>

// Mock data
typedef struct {
    const char* name;
    int value;
} mock_item;

mock_item items[] = {
    {"foo", 10},
    {"bar", 20}
};

int mock_count(const void* parent) { return 2; }
const void* mock_lookup(const void* parent, const char* name) { return NULL; }
const char* mock_key(const void* parent, int i) {
    if (i < 0 || i >= 2) return NULL;
    return items[i].name;
}
const void* mock_value(const void* parent, int i) {
    if (i < 0 || i >= 2) return NULL;
    return &items[i];
}
SV* mock_wrap(pTHX_ const void* item) {
    mock_item* mi = (mock_item*)item;
    return newSViv(mi->value);
}

static const PerlUpb_ByNameMap_VTable mock_vtable = {
    mock_count, mock_lookup, mock_key, mock_value, mock_wrap
};

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(8);

    SV* parent_sv = newSViv(1);
    SV* map_sv = PerlUpb_ByNameMap_New(aTHX_ parent_sv, NULL, &mock_vtable);

    SV* iter_sv = PerlUpb_DescriptorMapIterator_New(aTHX_ map_sv);
    ok(iter_sv != NULL, "Created MapIterator");
    ok(sv_derived_from(iter_sv, "Protobuf::Internals::DescriptorMapIterator"), "Blessed correctly");

    // Iteration 1
    SV* key1 = PerlUpb_DescriptorMapIterator_NextKey(aTHX_ iter_sv);
    is_string(SvPV_nolen(key1), "foo", "NextKey 1 is 'foo'");
    SV* val1 = PerlUpb_DescriptorMapIterator_NextValue(aTHX_ iter_sv);
    is(SvIV(val1), 10, "NextValue 1 is 10");
    SvREFCNT_dec(key1);
    SvREFCNT_dec(val1);

    // Iteration 2
    SV* key2 = PerlUpb_DescriptorMapIterator_NextKey(aTHX_ iter_sv);
    is_string(SvPV_nolen(key2), "bar", "NextKey 2 is 'bar'");
    SV* val2 = PerlUpb_DescriptorMapIterator_NextValue(aTHX_ iter_sv);
    is(SvIV(val2), 20, "NextValue 2 is 20");
    SvREFCNT_dec(key2);
    SvREFCNT_dec(val2);

    // End
    SV* key3 = PerlUpb_DescriptorMapIterator_NextKey(aTHX_ iter_sv);
    ok(!SvOK(key3), "NextKey 3 is undef");
    SvREFCNT_dec(key3);

    // Cleanup
    extern void PerlUpb_DescriptorMapIterator_Free(pTHX_ SV* sv);
    PerlUpb_DescriptorMapIterator_Free(aTHX_ iter_sv);
    SvREFCNT_dec(iter_sv);

    extern void PerlUpb_ByNameMap_Free(pTHX_ SV* sv);
    PerlUpb_ByNameMap_Free(aTHX_ map_sv);
    SvREFCNT_dec(map_sv);
    SvREFCNT_dec(parent_sv);

    test_perl_destroy(my_perl);
    return 0;
}
