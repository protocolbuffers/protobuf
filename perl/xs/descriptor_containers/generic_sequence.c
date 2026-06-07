#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_containers/generic_sequence.h"

SV* PerlUpb_GenericSequence_New(pTHX_ SV* parent_sv, const void* parent, const PerlUpb_GenericSequence_VTable* vtable) {
    PerlUpb_GenericSequence* seq = (PerlUpb_GenericSequence*)malloc(sizeof(PerlUpb_GenericSequence));
    seq->parent = parent;
    seq->parent_sv = newSVsv(parent_sv);
    seq->vtable = vtable;

    SV* sv = newSViv((IV)seq);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::Internals::DescriptorSequence", GV_ADD));
    return obj;
}

PerlUpb_GenericSequence* PerlUpb_GenericSequence_Get(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::Internals::DescriptorSequence")) {
        return NULL;
    }
    return (PerlUpb_GenericSequence*)SvIV(SvRV(sv));
}

int PerlUpb_GenericSequence_Count(pTHX_ SV* self) {
    PerlUpb_GenericSequence* seq = PerlUpb_GenericSequence_Get(aTHX_ self);
    if (!seq) return 0;
    return seq->vtable->count(seq->parent);
}

SV* PerlUpb_GenericSequence_GetItem(pTHX_ SV* self, int index) {
    PerlUpb_GenericSequence* seq = PerlUpb_GenericSequence_Get(aTHX_ self);
    if (!seq) return &PL_sv_undef;
    const void* item = seq->vtable->get(seq->parent, index);
    if (!item) return &PL_sv_undef;
    return seq->vtable->wrap(aTHX_ item);
}

void PerlUpb_GenericSequence_Free(pTHX_ SV* sv) {
    PerlUpb_GenericSequence* seq = PerlUpb_GenericSequence_Get(aTHX_ sv);
    if (seq) {
        SvREFCNT_dec(seq->parent_sv);
        free(seq);
        sv_setiv(SvRV(sv), 0);
    }
}
