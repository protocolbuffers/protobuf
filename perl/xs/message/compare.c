#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/message/compare.h"
#include "xs/protobuf/message.h"
#include "upb/message/compare.h"
#include "upb/reflection/def.h"

bool PerlUpb_Message_IsEqual(pTHX_ SV* message1_sv, SV* message2_sv) {
    const upb_Message* msg1 = PerlUpb_Message_GetMsg(aTHX_ message1_sv);
    const upb_Message* msg2 = PerlUpb_Message_GetMsg(aTHX_ message2_sv);

    if (!msg1 || !msg2) {
        return false; // Can't compare null messages
    }

    const upb_MessageDef* mdef1 = PerlUpb_Message_GetDef(aTHX_ message1_sv);
    const upb_MessageDef* mdef2 = PerlUpb_Message_GetDef(aTHX_ message2_sv);

    if (mdef1 != mdef2) {
        return false; // Different types
    }

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef1);
    if (!mt) return false;

    return upb_Message_IsEqual(msg1, msg2, mt, 0);
}
