#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_containers/by_number_map.h"

SV* PerlUpb_ByNumberMap_New(pTHX_ SV* parent_sv, const void* parent, const PerlUpb_ByNumberMap_VTable* vtable) {
    PerlUpb_ByNumberMap* map = (PerlUpb_ByNumberMap*)malloc(sizeof(PerlUpb_ByNumberMap));
    map->parent = parent;
    map->parent_sv = newSVsv(parent_sv);
    map->vtable = vtable;

    SV* sv = newSViv((IV)map);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::Internals::DescriptorByNumberMap", GV_ADD));
    return obj;
}

PerlUpb_ByNumberMap* PerlUpb_ByNumberMap_Get(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::Internals::DescriptorByNumberMap")) {
        return NULL;
    }
    return (PerlUpb_ByNumberMap*)SvIV(SvRV(sv));
}

int PerlUpb_ByNumberMap_Count(pTHX_ SV* self) {
    PerlUpb_ByNumberMap* map = PerlUpb_ByNumberMap_Get(aTHX_ self);
    if (!map) return 0;
    return map->vtable->count(map->parent);
}

SV* PerlUpb_ByNumberMap_Lookup(pTHX_ SV* self, uint32_t num) {
    PerlUpb_ByNumberMap* map = PerlUpb_ByNumberMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    const void* item = map->vtable->lookup(map->parent, num);
    if (!item) return &PL_sv_undef;
    return map->vtable->wrap(aTHX_ item);
}

SV* PerlUpb_ByNumberMap_Key(pTHX_ SV* self, int index) {
    PerlUpb_ByNumberMap* map = PerlUpb_ByNumberMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    uint32_t key = map->vtable->key(map->parent, index);
    return newSVuv(key);
}

SV* PerlUpb_ByNumberMap_Value(pTHX_ SV* self, int index) {
    PerlUpb_ByNumberMap* map = PerlUpb_ByNumberMap_Get(aTHX_ self);
    if (!map) return &PL_sv_undef;
    const void* item = map->vtable->value(map->parent, index);
    if (!item) return &PL_sv_undef;
    return map->vtable->wrap(aTHX_ item);
}

void PerlUpb_ByNumberMap_Free(pTHX_ SV* sv) {
    PerlUpb_ByNumberMap* map = PerlUpb_ByNumberMap_Get(aTHX_ sv);
    if (map) {
        SvREFCNT_dec(map->parent_sv);
        free(map);
        sv_setiv(SvRV(sv), 0);
    }
}
