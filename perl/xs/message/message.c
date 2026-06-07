#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/message/message.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "xs/descriptor/message.h"
#include "upb/message/merge.h"
#include "upb/message/copy.h"
#include "upb/reflection/message.h"

SV* PerlUpb_Message_NewMessage(pTHX_ SV* descriptor_sv, uint16_t flags) {
    const upb_MessageDef *mdef = PerlUpb_MessageDef_GetMessage(aTHX_ descriptor_sv);
    if (!mdef) {
        croak("descriptor_sv must be a Protobuf::MessageDescriptor");
    }

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef);
    if (!mt) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to get MiniTable for message");
    }

    upb_Message *msg = upb_Message_New(mt, arena);
    if (!msg) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to allocate upb_Message");
    }

    SV *msg_sv = PerlUpb_WrapMessage_NoCache(aTHX_ msg, mdef, arena_sv, flags);
    SvREFCNT_dec(arena_sv);

    return msg_sv;
}

void PerlUpb_Message_MergeFrom(pTHX_ SV* dst_sv, SV* src_sv) {
    upb_Message* dst_msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ dst_sv);
    const upb_Message* src_msg = PerlUpb_Message_GetMsg(aTHX_ src_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ dst_sv);
    const upb_MessageDef* src_mdef = PerlUpb_Message_GetDef(aTHX_ src_sv);

    if (!dst_msg || !src_msg || !mdef || !src_mdef) croak("Invalid message objects");
    if (mdef != src_mdef) croak("Messages must have the same descriptor for MergeFrom");

    SV* dst_arena_sv = PerlUpb_Message_GetArena(aTHX_ dst_sv);
    upb_Arena* dst_arena = PerlUpb_Arena_Get(aTHX_ dst_arena_sv);

    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);
    const upb_FileDef* file = upb_MessageDef_File(mdef);
    const upb_ExtensionRegistry* extreg = (const upb_ExtensionRegistry*)upb_FileDef_Pool(file);

    if (!upb_Message_MergeFrom(dst_msg, src_msg, mt, extreg, dst_arena)) {
        croak("Failed to merge messages");
    }
}

void PerlUpb_Message_CopyFrom(pTHX_ SV* dst_sv, SV* src_sv) {
    upb_Message* dst_msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ dst_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ dst_sv);
    if (!dst_msg || !mdef) croak("Invalid destination message object");

    // Clear destination first
    upb_Message_ClearByDef(dst_msg, mdef);

    // Then merge
    PerlUpb_Message_MergeFrom(aTHX_ dst_sv, src_sv);
}
