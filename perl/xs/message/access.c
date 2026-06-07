#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/message/access.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/arena.h"
#include "xs/convert.h"
#include "upb/reflection/message.h"

SV* PerlUpb_Message_GetField(pTHX_ SV* message_sv, const upb_FieldDef* f) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (!msg) croak("Invalid message object");

    bool needs_arena = upb_FieldDef_IsSubMessage(f) ||
                      upb_FieldDef_IsRepeated(f) ||
                      upb_FieldDef_IsMap(f);

    if (needs_arena) {
        SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ message_sv);
        upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

        if (upb_FieldDef_IsRepeated(f) || upb_FieldDef_IsMap(f)) {
            upb_MutableMessageValue mutable_val = upb_Message_Mutable((upb_Message*)msg, f, arena);
            upb_MessageValue val;
            if (upb_FieldDef_IsMap(f)) {
                val.map_val = mutable_val.map;
            } else {
                val.array_val = mutable_val.array;
            }
            return PerlUpb_UpbToSv(aTHX_ &val, f, arena_sv);
        } else {
            // Sub-message
            upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
            return PerlUpb_UpbToSv(aTHX_ &val, f, arena_sv);
        }
    } else {
        // Scalar path: Arena not needed for UpbToSv (including strings/bytes as they are copied to SV)
        upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
        return PerlUpb_UpbToSv(aTHX_ &val, f, NULL);
    }
}

void PerlUpb_Message_SetField(pTHX_ SV* message_sv, const upb_FieldDef* f, SV* val_sv) {
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (!msg) croak("Invalid message object");

    SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ message_sv);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    upb_MessageValue val;
    if (!PerlUpb_SvToUpb(aTHX_ val_sv, f, &val, arena)) {
        croak("Failed to convert value for setting field");
    }

    upb_Message_SetFieldByDef(msg, f, val, arena);
}

bool PerlUpb_Message_HasField(pTHX_ SV* message_sv, const upb_FieldDef* f) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (!msg) croak("Invalid message object");

    if (!upb_FieldDef_HasPresence(f)) {
        croak("Field does not have presence");
    }

    return upb_Message_HasFieldByDef(msg, f);
}

void PerlUpb_Message_ClearField(pTHX_ SV* message_sv, const upb_FieldDef* f) {
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (!msg) croak("Invalid message object");

    upb_Message_ClearFieldByDef(msg, f);
}

const char* PerlUpb_Message_WhichOneof(pTHX_ SV* message_sv, const upb_OneofDef* o) {
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (!msg) return NULL;

    const upb_FieldDef* f = upb_Message_WhichOneofByDef(msg, o);
    return f ? upb_FieldDef_Name(f) : "";
}

void PerlUpb_Message_Clear(pTHX_ SV* message_sv) {
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    upb_Message_ClearByDef(msg, mdef);
}

#include "xs/descriptor/field.h"

SV* PerlUpb_Message_Fields(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    AV* av = newAV();
    const upb_FieldDef* f;
    upb_MessageValue val;
    size_t iter = kUpb_Message_Begin;

    while (upb_Message_Next(msg, mdef, NULL, &f, &val, &iter)) {
        av_push(av, PerlUpb_FieldDef_GetWrapper(aTHX_ f));
    }

    return newRV_noinc((SV*)av);
}

