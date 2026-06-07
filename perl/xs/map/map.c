#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/map/map.h"
#include "xs/protobuf/obj_cache.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/utils.h"
#include "xs/convert.h"
#include "xs/protobuf/registry.h"
#include "upb/message/map.h"
#include "upb/reflection/def.h"

SV* PerlUpb_Map_New(pTHX_ upb_Map* map, const upb_FieldDef* f, SV* arena_sv, uint16_t flags) {
    if (!map) return &PL_sv_undef;

    SV* cached = PerlUpb_ObjCache_Get(aTHX_ map);
    if (cached) return cached;

    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    HV* stash = (reg && reg->stash_map) ? reg->stash_map : gv_stashpv("Protobuf::Internal::Map", GV_ADD);
    SV* self = PerlUpb_WrapArenaBoundObject(aTHX_ map, arena_sv, stash, flags);
    HV* hv = (HV*)SvRV(self);
    hv_store(hv, "_fdef", 5, newSViv(PTR2IV(f)), 0);

    return self;
}

static upb_Map* GetMap(pTHX_ SV* sv) {
    return (upb_Map*)PerlUpb_GetArenaBoundObject(aTHX_ sv, "Protobuf::Internal::Map");
}

static const upb_FieldDef* GetFieldDef(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv)) return NULL;
    HV* hv = (HV*)SvRV(sv);
    SV** svp = hv_fetch(hv, "_fdef", 5, 0);
    return svp ? (const upb_FieldDef*)SvIV(*svp) : NULL;
}

static void GetMapEntryDefs(const upb_FieldDef* f, const upb_FieldDef** key_f, const upb_FieldDef** val_f) {
    const upb_MessageDef* entry_def = upb_FieldDef_MessageSubDef(f);
    *key_f = upb_MessageDef_FindFieldByNumber(entry_def, 1);
    *val_f = upb_MessageDef_FindFieldByNumber(entry_def, 2);
}

SV* PerlUpb_Map_GetItem(pTHX_ SV* self, SV* key_sv) {
    upb_Map* map = GetMap(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!map || !f) return &PL_sv_undef;

    const upb_FieldDef *key_f, *val_f;
    GetMapEntryDefs(f, &key_f, &val_f);

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue key_val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ key_sv, key_f, &key_val, arena)) {
        croak("Invalid map key type");
    }

    upb_MessageValue val;
    if (upb_Map_Get(map, key_val, &val)) {
        return PerlUpb_UpbToSv_Element(aTHX_ &val, val_f, arena_sv);
    }

    return &PL_sv_undef;
}

bool PerlUpb_Map_Exists(pTHX_ SV* self, SV* key_sv) {
    upb_Map* map = GetMap(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!map || !f) return false;

    const upb_FieldDef *key_f, *val_f;
    GetMapEntryDefs(f, &key_f, &val_f);

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue key_val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ key_sv, key_f, &key_val, arena)) {
        return false;
    }

    upb_MessageValue val;
    return upb_Map_Get(map, key_val, &val);
}

void PerlUpb_Map_SetItem(pTHX_ SV* self, SV* key_sv, SV* value_sv) {
    upb_Map* map = GetMap(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!map || !f) return;

    const upb_FieldDef *key_f, *val_f;
    GetMapEntryDefs(f, &key_f, &val_f);

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue key_val, val;

    if (!PerlUpb_SvToUpb_Element(aTHX_ key_sv, key_f, &key_val, arena)) {
        croak("Invalid map key type");
    }
    if (!PerlUpb_SvToUpb_Element(aTHX_ value_sv, val_f, &val, arena)) {
        croak("Invalid map value type");
    }

    upb_Map_Set(map, key_val, val, arena);
}

void PerlUpb_Map_DeleteItem(pTHX_ SV* self, SV* key_sv) {
    upb_Map* map = GetMap(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!map || !f) return;

    const upb_FieldDef *key_f, *val_f;
    GetMapEntryDefs(f, &key_f, &val_f);

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_MessageValue key_val;
    if (!PerlUpb_SvToUpb_Element(aTHX_ key_sv, key_f, &key_val, arena)) {
        croak("Invalid map key type");
    }

    upb_Map_Delete(map, key_val, NULL);
}

void PerlUpb_Map_Clear(pTHX_ SV* self) {
    upb_Map* map = GetMap(aTHX_ self);
    if (map) {
        upb_Map_Clear(map);
    }
}

int PerlUpb_Map_Size(pTHX_ SV* self) {
    upb_Map* map = GetMap(aTHX_ self);
    return map ? upb_Map_Size(map) : 0;
}

SV* PerlUpb_Map_AsHash(pTHX_ SV* self) {
    upb_Map* map = GetMap(aTHX_ self);
    const upb_FieldDef* f = GetFieldDef(aTHX_ self);
    if (!map || !f) return &PL_sv_undef;

    HV* hv = newHV();
    const upb_FieldDef *key_f, *val_f;
    GetMapEntryDefs(f, &key_f, &val_f);

    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);
    size_t iter = kUpb_Map_Begin;
    upb_MessageValue k, v;
    while (upb_Map_Next(map, &k, &v, &iter)) {
        SV* k_sv = PerlUpb_UpbToSv_Element(aTHX_ &k, key_f, arena_sv);
        SV* v_sv = PerlUpb_UpbToSv_Element(aTHX_ &v, val_f, arena_sv);

        STRLEN len;
        char* key_str;
        if (upb_FieldDef_Type(key_f) == kUpb_FieldType_String) {
            key_str = SvPVutf8(k_sv, len);
        } else {
            key_str = SvPVbyte(k_sv, len);
        }
        hv_store(hv, key_str, len, v_sv, 0);
        SvREFCNT_dec(k_sv);
    }

    return newRV_noinc((SV*)hv);
}

void PerlUpb_Map_Free(pTHX_ SV* sv) {
    // No-op, handled by magic wrapper_cleanup
}

const upb_FieldDef* PerlUpb_Map_GetFieldDef(pTHX_ SV* self) {
    return GetFieldDef(aTHX_ self);
}

upb_Map* PerlUpb_Map_GetMapPtr(pTHX_ SV* self) {
    return GetMap(aTHX_ self);
}

SV* PerlUpb_Map_GetArenaSV(pTHX_ SV* self) {
    return PerlUpb_GetArenaFromObject(aTHX_ self);
}
