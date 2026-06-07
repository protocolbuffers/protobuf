#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_containers/iterators.h"
#include "xs/descriptor_containers/by_name_map.h"
#include "xs/descriptor_containers/by_number_map.h"

SV* PerlUpb_DescriptorMapIterator_New(pTHX_ SV* container_sv) {
    PerlUpb_DescriptorMapIterator* iter = (PerlUpb_DescriptorMapIterator*)malloc(sizeof(PerlUpb_DescriptorMapIterator));
    iter->container_sv = newSVsv(container_sv);
    iter->index = 0;

    SV* sv = newSViv((IV)iter);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::Internals::DescriptorMapIterator", GV_ADD));
    return obj;
}

PerlUpb_DescriptorMapIterator* PerlUpb_DescriptorMapIterator_Get(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::Internals::DescriptorMapIterator")) {
        return NULL;
    }
    return (PerlUpb_DescriptorMapIterator*)SvIV(SvRV(sv));
}

SV* PerlUpb_DescriptorMapIterator_NextKey(pTHX_ SV* self) {
    PerlUpb_DescriptorMapIterator* iter = PerlUpb_DescriptorMapIterator_Get(aTHX_ self);
    if (!iter) return &PL_sv_undef;

    // Try ByNameMap first
    PerlUpb_ByNameMap* name_map = PerlUpb_ByNameMap_Get(aTHX_ iter->container_sv);
    if (name_map) {
        if (iter->index >= name_map->vtable->count(name_map->parent)) {
            return &PL_sv_undef;
        }
        return PerlUpb_ByNameMap_Key(aTHX_ iter->container_sv, iter->index++);
    }

    // Then ByNumberMap
    PerlUpb_ByNumberMap* num_map = PerlUpb_ByNumberMap_Get(aTHX_ iter->container_sv);
    if (num_map) {
        if (iter->index >= num_map->vtable->count(num_map->parent)) {
            return &PL_sv_undef;
        }
        return PerlUpb_ByNumberMap_Key(aTHX_ iter->container_sv, iter->index++);
    }

    return &PL_sv_undef;
}

// Similar for NextValue, but we don't increment index here
// The Perl layer would call NextKey, then NextValue for each 'each' iteration.
// Or we could have a 'Next' that returns both.
// For now, let's just make NextKey increment and have a separate GetValue for the CURRENT index.

SV* PerlUpb_DescriptorMapIterator_NextValue(pTHX_ SV* self) {
    PerlUpb_DescriptorMapIterator* iter = PerlUpb_DescriptorMapIterator_Get(aTHX_ self);
    if (!iter || iter->index == 0) return &PL_sv_undef;

    int current_index = iter->index - 1;

    PerlUpb_ByNameMap* name_map = PerlUpb_ByNameMap_Get(aTHX_ iter->container_sv);
    if (name_map) {
        return PerlUpb_ByNameMap_Value(aTHX_ iter->container_sv, current_index);
    }

    PerlUpb_ByNumberMap* num_map = PerlUpb_ByNumberMap_Get(aTHX_ iter->container_sv);
    if (num_map) {
        return PerlUpb_ByNumberMap_Value(aTHX_ iter->container_sv, current_index);
    }

    return &PL_sv_undef;
}

void PerlUpb_DescriptorMapIterator_Free(pTHX_ SV* sv) {
    PerlUpb_DescriptorMapIterator* iter = PerlUpb_DescriptorMapIterator_Get(aTHX_ sv);
    if (iter) {
        SvREFCNT_dec(iter->container_sv);
        free(iter);
        sv_setiv(SvRV(sv), 0);
    }
}
