#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_containers/by_name_map.h"
#include "xs/protobuf/obj_cache.h"

SV* PerlUpb_ByNameMap_New(pTHX_ SV* parent_sv, const void* parent, const PerlUpb_ByNameMap_VTable* vtable) {
    PerlUpb_ByNameMap* map = (PerlUpb_ByNameMap*)malloc(sizeof(PerlUpb_ByNameMap));
    map->parent = parent;
    map->parent_sv = newSVsv(parent_sv);
    map->vtable = vtable;

    SV* sv = newSViv((IV)map);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::Internals::DescriptorByNameMap", GV_ADD));

    // We don't cache these as they are transient wrappers,
    // though Python does cache them. For now, let's keep it simple.

    return obj;
}

PerlUpb_ByNameMap* PerlUpb_ByNameMap_Get(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::Internals::DescriptorByNameMap")) {
        return NULL;
    }
    return (PerlUpb_ByNameMap*)SvIV(SvRV(sv));
}

int PerlUpb_ByNameMap_Count(pTHX_ SV* self) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ self);
    if (!map) return 0;
    return map->vtable->count(map->parent);
}

SV* PerlUpb_ByNameMap_Lookup(pTHX_ SV* self, const char* name) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    const void* item = map->vtable->lookup(map->parent, name);
    if (!item) return &PL_sv_undef;
    return map->vtable->wrap(aTHX_ item);
}

SV* PerlUpb_ByNameMap_Key(pTHX_ SV* self, int index) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    const char* key = map->vtable->key(map->parent, index);
    if (!key) return &PL_sv_undef;
    return newSVpv(key, 0);
}

SV* PerlUpb_ByNameMap_Value(pTHX_ SV* self, int index) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    const void* item = map->vtable->value(map->parent, index);
    if (!item) return &PL_sv_undef;
    return map->vtable->wrap(aTHX_ item);
}

SV* PerlUpb_ByNameMap_AsHash(pTHX_ SV* self) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;

    HV* hv = newHV();
    int count = map->vtable->count(map->parent);
    for (int i = 0; i < count; i++) {
        const char* key = map->vtable->key(map->parent, i);
        const void* item = map->vtable->value(map->parent, i);
        if (key && item) {
            SV* val = map->vtable->wrap(aTHX_ item);
            hv_store(hv, key, strlen(key), val, 0);
        }
    }
    return newRV_noinc((SV*)hv);
}

// Cleanup function (to be called from DESTROY)
void PerlUpb_ByNameMap_Free(pTHX_ SV* sv) {
    PerlUpb_ByNameMap* map = PerlUpb_ByNameMap_Get(aTHX_ sv);
    if (map) {
        SvREFCNT_dec(map->parent_sv);
        free(map);
        sv_setiv(SvRV(sv), 0);
    }
}
