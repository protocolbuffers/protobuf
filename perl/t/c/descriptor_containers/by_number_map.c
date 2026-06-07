#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/descriptor_containers/by_number_map.h"
#include <stdio.h>

// Mock data
typedef struct {
    uint32_t number;
    int value;
} mock_item;

mock_item items[] = {
    {1, 100},
    {2, 200},
    {3, 300}
};

int mock_count(const void* parent) { return 3; }
const void* mock_lookup(const void* parent, uint32_t num) {
    for (int i = 0; i < 3; i++) {
        if (items[i].number == num) return &items[i];
    }
    return NULL;
}
uint32_t mock_key(const void* parent, int i) {
    if (i < 0 || i >= 3) return 0;
    return items[i].number;
}
const void* mock_value(const void* parent, int i) {
    if (i < 0 || i >= 3) return NULL;
    return &items[i];
}
SV* mock_wrap(pTHX_ const void* item) {
    mock_item* mi = (mock_item*)item;
    return newSViv(mi->value);
}

static const PerlUpb_ByNumberMap_VTable mock_vtable = {
    mock_count, mock_lookup, mock_key, mock_value, mock_wrap
};

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(8);

    SV* parent_sv = newSViv(1);
    SV* map_sv = PerlUpb_ByNumberMap_New(aTHX_ parent_sv, NULL, &mock_vtable);
    ok(map_sv != NULL, "Created ByNumberMap");
    ok(sv_derived_from(map_sv, "Protobuf::Internals::DescriptorByNumberMap"), "Blessed correctly");

    is(PerlUpb_ByNumberMap_Count(aTHX_ map_sv), 3, "Count is 3");

    SV* val1_lookup = PerlUpb_ByNumberMap_Lookup(aTHX_ map_sv, 1);
    ok(SvIOK(val1_lookup), "Lookup returns IOK");
    is(SvIV(val1_lookup), 100, "Value for number 1 is 100");
    SvREFCNT_dec(val1_lookup);

    SV* key2 = PerlUpb_ByNumberMap_Key(aTHX_ map_sv, 2);
    is(SvUV(key2), 3, "Key at index 2 is 3");
    SvREFCNT_dec(key2);

    SV* val2 = PerlUpb_ByNumberMap_Value(aTHX_ map_sv, 2);
    is(SvIV(val2), 300, "Value at index 2 is 300");
    SvREFCNT_dec(val2);

    TODO reverse lookup for ByNumberMap") {
        ok(0, "Value-to-Key lookups optimized via internal reverse-index");
    }

    // Cleanup
    extern void PerlUpb_ByNumberMap_Free(pTHX_ SV* sv);
    PerlUpb_ByNumberMap_Free(aTHX_ map_sv);
    SvREFCNT_dec(map_sv);
    SvREFCNT_dec(parent_sv);

    test_perl_destroy(my_perl);
    return 0;
}
