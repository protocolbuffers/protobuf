#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/protobuf/obj_cache.h"

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);
    {
        dTHX;
        plan(6);

        PerlUpb_ObjCache_Init(aTHX);
        ok(1, "Cache initialized");

        // Set capacity to 3
        PerlUpb_ObjCache_SetCapacity(aTHX_ 3);
        is(PerlUpb_ObjCache_GetCapacity(aTHX), 3, "Capacity set to 3");

        int objs[5];
        SV* rvs[5];
        for (int i = 0; i < 5; i++) {
            rvs[i] = newRV_noinc(newSViv(i));
        }

        // Add 3 items
        PerlUpb_ObjCache_Add(aTHX_ &objs[0], rvs[0]);
        PerlUpb_ObjCache_Add(aTHX_ &objs[1], rvs[1]);
        PerlUpb_ObjCache_Add(aTHX_ &objs[2], rvs[2]);

        SV* got = PerlUpb_ObjCache_Get(aTHX_ &objs[0]);
        ok(got != NULL, "Item 0 still in cache");
        if (got) SvREFCNT_dec(got);

        // Add 4th item -> should evict item 0
        PerlUpb_ObjCache_Add(aTHX_ &objs[3], rvs[3]);

        got = PerlUpb_ObjCache_Get(aTHX_ &objs[0]);
        ok(got == NULL, "Item 0 evicted from cache");

        got = PerlUpb_ObjCache_Get(aTHX_ &objs[1]);
        ok(got != NULL, "Item 1 still in cache");
        if (got) SvREFCNT_dec(got);

        // Add 5th item -> should evict item 1
        PerlUpb_ObjCache_Add(aTHX_ &objs[4], rvs[4]);

        got = PerlUpb_ObjCache_Get(aTHX_ &objs[1]);
        ok(got == NULL, "Item 1 evicted from cache");

        for (int i = 0; i < 5; i++) {
            SvREFCNT_dec(rvs[i]);
        }

        PerlUpb_ObjCache_Clear(aTHX);
    }
    test_perl_destroy(my_perl);
    return 0;
}
