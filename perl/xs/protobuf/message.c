#include "xs/protobuf/message.h"
#include "xs/protobuf.h"
#include "upb/reflection/def.h"
#include "xs/protobuf/obj_cache.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/utils.h"

static int descriptor_cleanup(pTHX_ SV* sv, MAGIC* mg) {
    // No explicit cleanup needed for the descriptor IV,
    // but having the magic ensures we don't trigger the "unreferenced scalar"
    // warnings during global destruction by explicitly managing it.
    return 0;
}

static SV* wrap_message_internal(pTHX_ const upb_Message* msg, const upb_MessageDef* mdef, SV* arena_sv, uint16_t flags) {
    HV* stash = PerlUpb_GetMessageStash(aTHX_ mdef);
    SV *self = PerlUpb_WrapArenaBoundObject(aTHX_ msg, arena_sv, stash, flags);

    // Store the descriptor C pointer in the HV.
    HV* hv = (HV*)SvRV(self);
    hv_store(hv, "_mdef_ptr", 9, newSViv(PTR2IV(mdef)), 0);

    return self;
}

SV *PerlUpb_WrapMessage(pTHX_ const upb_Message *msg, const upb_MessageDef *mdef, SV *arena_sv, uint16_t flags) {
    if (!msg) return &PL_sv_undef;

    SV* cached = PerlUpb_ObjCache_Get(aTHX_ msg);
    if (cached) return cached;

    return wrap_message_internal(aTHX_ msg, mdef, arena_sv, flags);
}

SV *PerlUpb_WrapMessage_NoCache(pTHX_ const upb_Message *msg, const upb_MessageDef *mdef, SV *arena_sv, uint16_t flags) {
    if (!msg) return &PL_sv_undef;
    return wrap_message_internal(aTHX_ msg, mdef, arena_sv, flags);
}

SV* PerlUpb_MaybeGetMessage(pTHX_ const upb_Message *msg) {
    if (!msg) return NULL;
    return PerlUpb_ObjCache_Get(aTHX_ msg);
}

void PerlUpb_Message_Free(pTHX_ SV *message_sv) {
    if (PL_dirty) return; // Let Perl handle cleanup during global destruction

    const upb_Message *msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    if (msg) {
        PerlUpb_ObjCache_Delete(aTHX_ msg);
    }
}

const upb_Message* PerlUpb_Message_GetMsg(pTHX_ SV* message_sv) {
    return (const upb_Message*)PerlUpb_GetArenaBoundObject(aTHX_ message_sv, "Protobuf::Message");
}

const upb_MessageDef* PerlUpb_Message_GetDef(pTHX_ SV* message_sv) {
    if (!message_sv || !SvROK(message_sv)) return NULL;
    HV* hv = (HV*)SvRV(message_sv);
    SV** svp = hv_fetch(hv, "_mdef_ptr", 9, 0);
    return (svp && SvIOK(*svp)) ? INT2PTR(const upb_MessageDef*, SvIV(*svp)) : NULL;
}

SV* PerlUpb_Message_GetArena(pTHX_ SV* message_sv) {
    return PerlUpb_GetArenaFromObject(aTHX_ message_sv);
}
