#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#include "xs/protobuf/message.h"
#include "xs/message/access.h"
#include "xs/descriptor/field.h"
#include "upb/reflection/def.h"

// The fast accessor XSUB implementation
static XS(PerlUpb_FastAccessor) {
    dXSARGS;

    // The field def pointer is stored in the CV's ANY pointer
    // 'cv' is automatically provided as a parameter to the XS function
    const upb_FieldDef* f = (const upb_FieldDef*)CvXSUBANY(cv).any_ptr;

    if (items < 1) croak("Usage: $msg->field([value])");
    SV* self = ST(0);

    if (!PerlUpb_IsXSBacked(aTHX_ self)) {
        const char* name = upb_FieldDef_Name(f);
        dSP;
        PUSHMARK(SP);
        XPUSHs(self);
        if (items > 1) {
            // Setter mode fallback: $msg->set('field', $val)
            XPUSHs(sv_2mortal(newSVpv(name, 0)));
            XPUSHs(ST(1));
            PUTBACK;
            call_method("set", G_DISCARD);
            SPAGAIN;
            XSRETURN_EMPTY;
        } else {
            // Getter mode fallback: $msg->get('field')
            XPUSHs(sv_2mortal(newSVpv(name, 0)));
            PUTBACK;
            int count = call_method("get", G_SCALAR);
            SPAGAIN;
            if (count != 1) croak("get method failed");
            ST(0) = POPs;
            XSRETURN(1);
        }
    }

    if (items > 1) {
        // Setter mode
        PerlUpb_Message_SetField(aTHX_ self, f, ST(1));

        // Mark cache as dirty for coarse invalidation
        MAGIC* mg = PerlUpb_GetMagic(aTHX_ self);
        if (mg) {
            // Bypass invalidation if WRITE_HEAVY (since we never use the cache)
            if (!(mg->mg_private & PERL_UPB_MG_PROFILE_WRITE_HEAVY)) {
                mg->mg_private |= PERL_UPB_MG_CACHE_DIRTY;
            }
        }

        XSRETURN_EMPTY;
    } else {
        // Getter mode
        MAGIC* mg = PerlUpb_GetMagic(aTHX_ self);

        // 0. Quick path for WRITE_HEAVY: bypass cache entirely
        if (mg && (mg->mg_private & PERL_UPB_MG_PROFILE_WRITE_HEAVY)) {
            ST(0) = PerlUpb_Message_GetField(aTHX_ self, f);
            XSRETURN(1);
        }

        HV* hv = (HV*)SvRV(self);

        // 1. Check for coarse invalidation
        if (mg && (mg->mg_private & PERL_UPB_MG_CACHE_DIRTY)) {
            hv_delete(hv, "_cache", 6, G_DISCARD);
            mg->mg_private &= ~PERL_UPB_MG_CACHE_DIRTY;
        }

        const char* name = upb_FieldDef_Name(f);
        STRLEN name_len = strlen(name);

        // 1. Fetch or create the nested _cache hash
        HV* cache_hv = NULL;
        SV** cache_svp = hv_fetch(hv, "_cache", 6, 0);
        if (cache_svp && SvROK(*cache_svp) && SvTYPE(SvRV(*cache_svp)) == SVt_PVHV) {
            cache_hv = (HV*)SvRV(*cache_svp);
        } else {
            cache_hv = newHV();
            SV* cache_rv = newRV_noinc((SV*)cache_hv);
            if (!hv_store(hv, "_cache", 6, cache_rv, 0)) {
                SvREFCNT_dec(cache_rv);
                // Fallback to non-cached path if store fails (unlikely)
                ST(0) = PerlUpb_Message_GetField(aTHX_ self, f);
                XSRETURN(1);
            }
        }

        // 2. Check memoization cache
        SV** cached = hv_fetch(cache_hv, name, name_len, 0);
        if (cached && SvOK(*cached)) {
            ST(0) = *cached;
            XSRETURN(1);
        }

        SV* result = PerlUpb_Message_GetField(aTHX_ self, f);

        // 3. Store in memoization cache
        SvREFCNT_inc(result);
        if (!hv_store(cache_hv, name, name_len, result, 0)) {
            SvREFCNT_dec(result);
        }

        ST(0) = result;
        XSRETURN(1);
    }
}

// Specialized setter for performance
static XS(PerlUpb_FastSetter) {
    dXSARGS;
    const upb_FieldDef* f = (const upb_FieldDef*)CvXSUBANY(cv).any_ptr;
    if (items != 2) croak("Usage: $msg->set_field(value)");
    SV* self = ST(0);

    if (!PerlUpb_IsXSBacked(aTHX_ self)) {
        const char* name = upb_FieldDef_Name(f);
        dSP;
        PUSHMARK(SP);
        XPUSHs(self);
        XPUSHs(sv_2mortal(newSVpv(name, 0)));
        XPUSHs(ST(1));
        PUTBACK;
        call_method("set", G_DISCARD);
        SPAGAIN;
        XSRETURN_EMPTY;
    }

    PerlUpb_Message_SetField(aTHX_ self, f, ST(1));

    // Mark cache as dirty for coarse invalidation
    MAGIC* mg = PerlUpb_GetMagic(aTHX_ self);
    if (mg) {
        if (!(mg->mg_private & PERL_UPB_MG_PROFILE_WRITE_HEAVY)) {
            mg->mg_private |= PERL_UPB_MG_CACHE_DIRTY;
        }
    }

    XSRETURN_EMPTY;
}

// Specialized has_field
static XS(PerlUpb_FastHas) {
    dXSARGS;
    const upb_FieldDef* f = (const upb_FieldDef*)CvXSUBANY(cv).any_ptr;
    if (items != 1) croak("Usage: $msg->has_field()");
    SV* self = ST(0);

    if (!PerlUpb_IsXSBacked(aTHX_ self)) {
        const char* name = upb_FieldDef_Name(f);
        dSP;
        PUSHMARK(SP);
        XPUSHs(self);
        XPUSHs(sv_2mortal(newSVpv(name, 0)));
        PUTBACK;
        int count = call_method("has", G_SCALAR);
        SPAGAIN;
        if (count != 1) croak("has method failed");
        ST(0) = POPs;
        XSRETURN(1);
    }

    bool has = PerlUpb_Message_HasField(aTHX_ self, f);
    ST(0) = has ? &PL_sv_yes : &PL_sv_no;
    XSRETURN(1);
}

// Specialized clear_field
static XS(PerlUpb_FastClear) {
    dXSARGS;
    const upb_FieldDef* f = (const upb_FieldDef*)CvXSUBANY(cv).any_ptr;
    if (items != 1) croak("Usage: $msg->clear_field()");
    SV* self = ST(0);

    if (!PerlUpb_IsXSBacked(aTHX_ self)) {
        const char* name = upb_FieldDef_Name(f);
        dSP;
        PUSHMARK(SP);
        XPUSHs(self);
        XPUSHs(sv_2mortal(newSVpv(name, 0)));
        PUTBACK;
        call_method("clear", G_DISCARD);
        SPAGAIN;
        XSRETURN_EMPTY;
    }

    PerlUpb_Message_ClearField(aTHX_ self, f);

    // Mark cache as dirty for coarse invalidation
    MAGIC* mg = PerlUpb_GetMagic(aTHX_ self);
    if (mg) {
        if (!(mg->mg_private & PERL_UPB_MG_PROFILE_WRITE_HEAVY)) {
            mg->mg_private |= PERL_UPB_MG_CACHE_DIRTY;
        }
    }

    XSRETURN_EMPTY;
}

void PerlUpb_InstallFastAccessors(pTHX_ const char* perl_class, const upb_MessageDef* mdef) {
    int field_count = upb_MessageDef_FieldCount(mdef);

    for (int i = 0; i < field_count; i++) {
        const upb_FieldDef* f = upb_MessageDef_Field(mdef, i);
        const char* name = upb_FieldDef_Name(f);

        // 1. Install primary accessor (getter/setter)
        {
            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s::%s", perl_class, name);
            CV* cv = newXS(full_name, PerlUpb_FastAccessor, __FILE__);
            CvXSUBANY(cv).any_ptr = (void*)f;
        }

        // 2. Install explicit setter
        {
            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s::set_%s", perl_class, name);
            CV* cv = newXS(full_name, PerlUpb_FastSetter, __FILE__);
            CvXSUBANY(cv).any_ptr = (void*)f;
        }

        // 3. Install has_ (if applicable)
        if (upb_FieldDef_HasPresence(f)) {
            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s::has_%s", perl_class, name);
            CV* cv = newXS(full_name, PerlUpb_FastHas, __FILE__);
            CvXSUBANY(cv).any_ptr = (void*)f;
        }

        // 4. Install clear_
        {
            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s::clear_%s", perl_class, name);
            CV* cv = newXS(full_name, PerlUpb_FastClear, __FILE__);
            CvXSUBANY(cv).any_ptr = (void*)f;
        }
    }
}
