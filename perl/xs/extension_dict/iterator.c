#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/extension_dict/iterator.h"
#include "xs/extension_dict/dict.h"
#include "xs/protobuf/message.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/file.h"
#include "upb/reflection/message.h"

typedef struct {
    SV* message_sv;
    size_t iter;
} PerlUpb_ExtensionDictIterator;

SV* PerlUpb_ExtensionDict_GetIterator(pTHX_ SV* dict_sv) {
    SV* message_sv = PerlUpb_ExtensionDict_GetMessageSV(aTHX_ dict_sv);
    if (!message_sv) return &PL_sv_undef;

    PerlUpb_ExtensionDictIterator* iter = (PerlUpb_ExtensionDictIterator*)malloc(sizeof(PerlUpb_ExtensionDictIterator));
    iter->message_sv = newSVsv(message_sv);
    iter->iter = kUpb_Message_Begin;

    SV* sv = newSViv((IV)iter);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::ExtensionDictIterator", GV_ADD));
    return obj;
}

static PerlUpb_ExtensionDictIterator* GetIter(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::ExtensionDictIterator")) {
        return NULL;
    }
    return (PerlUpb_ExtensionDictIterator*)SvIV(SvRV(sv));
}

SV* PerlUpb_ExtensionDict_Iterator_Next(pTHX_ SV* self) {
    PerlUpb_ExtensionDictIterator* iter = GetIter(aTHX_ self);
    if (!iter) return &PL_sv_undef;

    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ iter->message_sv);
    if (!msg) return &PL_sv_undef;

    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ iter->message_sv);
    const upb_DefPool* pool = upb_FileDef_Pool(upb_MessageDef_File(mdef));

    const upb_FieldDef* f;
    upb_MessageValue val;

    while (upb_Message_Next(msg, mdef, pool, &f, &val, &iter->iter)) {
        if (upb_FieldDef_IsExtension(f)) {
            return PerlUpb_FieldDef_GetWrapper(aTHX_ f);
        }
    }

    return &PL_sv_undef;
}

void PerlUpb_ExtensionDictIterator_Free(pTHX_ SV* sv) {
    PerlUpb_ExtensionDictIterator* iter = GetIter(aTHX_ sv);
    if (iter) {
        SvREFCNT_dec(iter->message_sv);
        free(iter);
        sv_setiv(SvRV(sv), 0);
    }
}
