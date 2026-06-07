#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/descriptor_containers/by_name_map.h"
#include <stdio.h>

// Mock data
typedef struct {
    const char* name;
    int value;
} mock_item;

mock_item items[] = {
    {"foo", 10},
    {"bar", 20},
    {"baz", 30}
};

int mock_count(const void* parent) { return 3; }
const void* mock_lookup(const void* parent, const char* name) {
    for (int i = 0; i < 3; i++) {
        if (strcmp(items[i].name, name) == 0) return &items[i];
    }
    return NULL;
}
const char* mock_key(const void* parent, int i) {
    if (i < 0 || i >= 3) return NULL;
    return items[i].name;
}
const void* mock_value(const void* parent, int i) {
    if (i < 0 || i >= 3) return NULL;
    return &items[i];
}
SV* mock_wrap(pTHX_ const void* item) {
    mock_item* mi = (mock_item*)item;
    return newSViv(mi->value);
}

static const PerlUpb_ByNameMap_VTable mock_vtable = {
    mock_count, mock_lookup, mock_key, mock_value, mock_wrap
};

static void test_as_hash(pTHX_ SV* map_sv) {
    SV* hash_rv = PerlUpb_ByNameMap_AsHash(aTHX_ map_sv);
    ok(SvROK(hash_rv) && SvTYPE(SvRV(hash_rv)) == SVt_PVHV, "AsHash returns a hash reference");
    HV* hv = (HV*)SvRV(hash_rv);

    is(hv_iterinit(hv), 3, "Projected hash has 3 keys");

    SV** foo_ptr = hv_fetch(hv, "foo", 3, 0);
    ok(foo_ptr != NULL, "Found 'foo' in hash");
    if (foo_ptr) {
        is(SvIV(*foo_ptr), 10, "Value for 'foo' is 10");
    }

    SV** bar_ptr = hv_fetch(hv, "bar", 3, 0);
    ok(bar_ptr != NULL, "Found 'bar' in hash");
    if (bar_ptr) {
        is(SvIV(*bar_ptr), 20, "Value for 'bar' is 20");
    }

    SvREFCNT_dec(hash_rv);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(18);

    SV* parent_sv = newSViv(1); // Fake parent
    SV* map_sv = PerlUpb_ByNameMap_New(aTHX_ parent_sv, NULL, &mock_vtable);
    ok(map_sv != NULL, "Created ByNameMap");
    ok(sv_derived_from(map_sv, "Protobuf::Internals::DescriptorByNameMap"), "Blessed correctly");

    is(PerlUpb_ByNameMap_Count(aTHX_ map_sv), 3, "Count is 3");

    SV* foo_val = PerlUpb_ByNameMap_Lookup(aTHX_ map_sv, "foo");
    ok(SvIOK(foo_val), "Lookup returns IOK");
    is(SvIV(foo_val), 10, "Value for 'foo' is 10");
    SvREFCNT_dec(foo_val);

    SV* key1 = PerlUpb_ByNameMap_Key(aTHX_ map_sv, 1);
    is_string(SvPV_nolen(key1), "bar", "Key at index 1 is 'bar'");
    SvREFCNT_dec(key1);

    SV* val1 = PerlUpb_ByNameMap_Value(aTHX_ map_sv, 1);
    is(SvIV(val1), 20, "Value at index 1 is 20");
    SvREFCNT_dec(val1);

    test_as_hash(aTHX_ map_sv);

    TODO {
        ok(0, "Iterators remain valid during interleaved read-only access in coroutines");
    }

    TODO {
        ok(0, "Repeated access to the same container element returns the same cached SV wrapper");
    }

    TODO {
        ok(0, "Multiple descriptors retrieved in a single call to minimize XS transition overhead");
    }

    // Cleanup
    extern void PerlUpb_ByNameMap_Free(pTHX_ SV* sv);
    PerlUpb_ByNameMap_Free(aTHX_ map_sv);
    SvREFCNT_dec(map_sv);
    SvREFCNT_dec(parent_sv);

    test_perl_destroy(my_perl);
    return 0;
}
