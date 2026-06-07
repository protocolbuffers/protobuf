#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/repeated/repeated.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/obj_cache.h"
#include "xs/protobuf/utils.h"
#include "xs/convert.h"
#include "xs/protobuf/registry.h"
#include "upb/message/array.h"
#include "upb/reflection/def.h"

SV* PerlUpb_Repeated_New(pTHX_ upb_Array* arr, const upb_FieldDef* f, SV* arena_sv, uint16_t flags) {
    if (!arr) return &PL_sv_undef;

    SV* cached = PerlUpb_ObjCache_Get(aTHX_ arr);
    if (cached) return cached;

    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    HV* stash = (reg && reg->stash_repeated) ? reg->stash_repeated : gv_stashpv("Protobuf::Internal::Repeated", GV_ADD);
    SV* self = PerlUpb_WrapArenaBoundObject(aTHX_ arr, arena_sv, stash, flags);
    HV* hv = (HV*)SvRV(self);
    hv_store(hv, "_fdef", 5, newSViv(PTR2IV(f)), 0);

    return self;
}

static upb_Array* GetArray(pTHX_ SV* sv) {
    return (upb_Array*)PerlUpb_GetArenaBoundObject(aTHX_ sv, "Protobuf::Internal::Repeated");
}

static const upb_FieldDef* GetFieldDef(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv)) return NULL;
    HV* hv = (HV*)SvRV(sv);
    SV** svp = hv_fetch(hv, "_fdef", 5, 0);
    return svp ? (const upb_FieldDef*)SvIV(*svp) : NULL;
}

SV* PerlUpb_Repeated_GetItem(pTHX_ SV* self, int index) {
    upb_Array* arr = GetArray(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!arr || !f) return &PL_sv_undef;

    size_t size = upb_Array_Size(arr);
    if (index < 0 || (size_t)index >= size) {
        return &PL_sv_undef;
    }

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_MessageValue val = upb_Array_Get(arr, index);
    return PerlUpb_UpbToSv_Element(aTHX_ &val, f, arena_sv);
}

void PerlUpb_Repeated_SetItem(pTHX_ SV* self, int index, SV* val_sv) {
    upb_Array* arr = GetArray(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!arr || !f) return;

    size_t size = upb_Array_Size(arr);
    if (index < 0 || (size_t)index >= size) {
        croak("Index out of bounds for repeated field");
    }

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ val_sv, f, &val, arena)) {
        croak("Failed to convert value for repeated field");
    }

    upb_Array_Set(arr, index, val);
}

void PerlUpb_Repeated_Append(pTHX_ SV* self, SV* val_sv) {
    upb_Array* arr = GetArray(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!arr || !f) return;

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ val_sv, f, &val, arena)) {
        croak("Failed to convert value for repeated field");
    }

    if (!upb_Array_Append(arr, val, arena)) {
        croak("Failed to append to repeated field");
    }
}

void PerlUpb_Repeated_Delete(pTHX_ SV* self, int index, int count) {
    upb_Array* arr = GetArray(aTHX_ self);
    if (!arr) return;

    size_t size = upb_Array_Size(arr);
    if (index < 0 || (size_t)index > size) return;
    if (index + count > (int)size) count = size - index;
    if (count <= 0) return;

    upb_Array_Delete(arr, index, count);
}

int PerlUpb_Repeated_Size(pTHX_ SV* self) {
    upb_Array* arr = GetArray(aTHX_ self);
    return arr ? upb_Array_Size(arr) : 0;
}

void PerlUpb_Repeated_Insert(pTHX_ SV* self, int index, SV* val_sv) {
    upb_Array* arr = GetArray(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!arr || !f) return;

    size_t size = upb_Array_Size(arr);
    if (index < 0 || (size_t)index > size) {
        croak("Index out of bounds for repeated field insert");
    }

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    if (!upb_Array_Insert(arr, index, 1, arena)) {
        croak("Failed to insert into repeated field");
    }

    upb_MessageValue val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ val_sv, f, &val, arena)) {
        upb_Array_Delete(arr, index, 1);
        croak("Failed to convert value for repeated field insert");
    }

    upb_Array_Set(arr, index, val);
}

void PerlUpb_Repeated_Resize(pTHX_ SV* self, int size) {
    upb_Array* arr = GetArray(aTHX_ self);
    if (!arr) return;

    if (size < 0) croak("Negative size for repeated field resize");

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    if (!upb_Array_Resize(arr, size, arena)) {
        croak("Failed to resize repeated field");
    }
}

void PerlUpb_Repeated_Clear(pTHX_ SV* self) {
    upb_Array* arr = GetArray(aTHX_ self);
    if (arr) {
        upb_Array_Resize(arr, 0, NULL);
    }
}

upb_Array* PerlUpb_Repeated_GetArray(pTHX_ SV* self) {
    return GetArray(aTHX_ self);
}

const upb_FieldDef* PerlUpb_Repeated_GetFieldDef(pTHX_ SV* self) {
    return GetFieldDef(aTHX_ self);
}

bool PerlUpb_Repeated_AuditIntegrity(pTHX_ SV* self) {
    upb_Array* arr = GetArray(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!arr || !f) return false;

    if (upb_FieldDef_IsSubMessage(f)) {
        size_t size = upb_Array_Size(arr);
        for (size_t i = 0; i < size; i++) {
            upb_MessageValue val = upb_Array_Get(arr, i);
            if (val.msg_val) {
                SV* cached = PerlUpb_ObjCache_Get(aTHX_ val.msg_val);
                if (cached) {
                    SvREFCNT_dec(cached);
                }
            }
        }
    }

    return true;
}

void PerlUpb_Repeated_Free(pTHX_ SV* sv) {
    // No-op, handled by magic wrapper_cleanup
}
