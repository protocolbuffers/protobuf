#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/message/meta.h"
#include "xs/protobuf/message.h"
#include "xs/descriptor/message.h"

SV* PerlUpb_Message_GetDescriptor(pTHX_ SV* message_sv) {
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!mdef) return &PL_sv_undef;
    return PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
}

#include "xs/protobuf/obj_cache.h"
#include "upb/reflection/message.h"

bool PerlUpb_Message_AuditIntegrity(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) return false;

    // Shallow audit: check if reified objects in this message's Perl hash
    // still point to the correct underlying upb objects.
    HV* hv = (HV*)SvRV(message_sv);

    int n = upb_MessageDef_FieldCount(mdef);
    for (int i = 0; i < n; i++) {
        const upb_FieldDef* f = upb_MessageDef_Field(mdef, i);
        const char* name = upb_FieldDef_Name(f);

        SV** svp = hv_fetch(hv, name, strlen(name), 0);
        if (svp && SvROK(*svp)) {
            SV* inner = SvRV(*svp);
            upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);

            if (upb_FieldDef_IsSubMessage(f)) {
                if (val.msg_val == NULL) {
                    // Perl has an object but upb says field is empty?
                    // This could be a "phantom" reification or corruption.
                    return false;
                }
                // Verify identity if reified
                SV* cached = PerlUpb_ObjCache_Get(aTHX_ val.msg_val);
                if (cached) {
                    if (SvRV(cached) != inner) {
                        SvREFCNT_dec(cached);
                        return false; // Identity mismatch!
                    }
                    SvREFCNT_dec(cached);
                }
            }
            // ... similar checks for repeated and maps could go here ...
        }
    }

    return true;
}
