#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/extension_dict/dict.h"
#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/field.h"
#include "xs/protobuf/message.h"
#include "xs/convert.h"
#include "upb/reflection/message.h"

typedef struct {
    SV* message_sv;
} PerlUpb_ExtensionDict;

SV* PerlUpb_ExtensionDict_New(pTHX_ SV* message_sv) {
    PerlUpb_ExtensionDict* dict = (PerlUpb_ExtensionDict*)malloc(sizeof(PerlUpb_ExtensionDict));
    dict->message_sv = newSVsv(message_sv);

    SV* sv = newSViv((IV)dict);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::ExtensionDict", GV_ADD));
    return obj;
}

static PerlUpb_ExtensionDict* GetDict(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::ExtensionDict")) {
        return NULL;
    }
    return (PerlUpb_ExtensionDict*)SvIV(SvRV(sv));
}

SV* PerlUpb_ExtensionDict_GetItem(pTHX_ SV* self, SV* field_sv) {
    PerlUpb_ExtensionDict* dict = GetDict(aTHX_ self);
    if (!dict) return &PL_sv_undef;

    const upb_FieldDef* f = PerlUpb_FieldDef_GetField(aTHX_ field_sv);
    if (!f) croak("Argument must be a FieldDescriptor");

    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ dict->message_sv);
    if (!msg) return &PL_sv_undef;

    if (!upb_FieldDef_IsExtension(f)) {
        croak("Field is not an extension");
    }

    upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
    return PerlUpb_UpbToSv(aTHX_ &val, f, PerlUpb_Message_GetArena(aTHX_ dict->message_sv));
}

void PerlUpb_ExtensionDict_SetItem(pTHX_ SV* self, SV* field_sv, SV* value_sv) {
    PerlUpb_ExtensionDict* dict = GetDict(aTHX_ self);
    if (!dict) return;

    const upb_FieldDef* f = PerlUpb_FieldDef_GetField(aTHX_ field_sv);
    if (!f) croak("Argument must be a FieldDescriptor");

    if (!upb_FieldDef_IsExtension(f)) {
        croak("Field is not an extension");
    }

    SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ dict->message_sv);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ dict->message_sv);

    upb_MessageValue val;
    if (!PerlUpb_SvToUpb(aTHX_ value_sv, f, &val, arena)) {
        croak("Failed to convert value for extension field");
    }

    upb_Message_SetFieldByDef(msg, f, val, arena);
}

void PerlUpb_ExtensionDict_Free(pTHX_ SV* sv) {
    PerlUpb_ExtensionDict* dict = GetDict(aTHX_ sv);
    if (dict) {
        SvREFCNT_dec(dict->message_sv);
        free(dict);
        sv_setiv(SvRV(sv), 0);
    }
}

SV* PerlUpb_ExtensionDict_GetMessageSV(pTHX_ SV* self) {
    PerlUpb_ExtensionDict* dict = GetDict(aTHX_ self);
    return dict ? dict->message_sv : NULL;
}

void PerlUpb_ExtensionDict_AuditIdentity(pTHX_ SV* self) {
    PerlUpb_ExtensionDict* dict = GetDict(aTHX_ self);
    if (!dict) return;

    // Log a HIT event for the parent message to verify identity connection
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ dict->message_sv);
    PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_HIT, msg);
}
