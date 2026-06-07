#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/repeated/composite.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "upb/message/array.h"
#include "upb/reflection/def.h"

// Internal struct from repeated.c (should probably move to a private header if needed,
// but for now I'll just re-declare or use an accessor if I added one).
// Let's add accessors to repeated.h for internal use.
SV* PerlUpb_Repeated_Add(pTHX_ SV* self) {
    upb_Array* arr = PerlUpb_Repeated_GetArray(aTHX_ self);
    const upb_FieldDef* f = PerlUpb_Repeated_GetFieldDef(aTHX_ self);
    SV* arena_sv = PerlUpb_GetArenaFromObject(aTHX_ self);

    if (!arr || !f || !arena_sv) return &PL_sv_undef;

    if (!upb_FieldDef_IsSubMessage(f)) {
        croak("add() can only be called on repeated message fields");
    }

    const upb_MessageDef* mdef = upb_FieldDef_MessageSubDef(f);
    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    upb_Message* msg = upb_Message_New(mt, arena);
    if (!msg) croak("Failed to create new submessage");

    upb_MessageValue val;
    val.msg_val = msg;
    if (!upb_Array_Append(arr, val, arena)) {
        croak("Failed to append new message to array");
    }

    return PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
}
