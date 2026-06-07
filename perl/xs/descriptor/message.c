#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/message.h"
#include "xs/descriptor/file.h"

const upb_FieldDef* PerlUpb_MessageDef_FindFieldByNameWithSize(pTHX_ const upb_MessageDef *m, const char *name, size_t len) {
    // Manual iteration to debug UPB lookup
    int count = upb_MessageDef_FieldCount(m);
    for (int i = 0; i < count; i++) {
        const upb_FieldDef* f = upb_MessageDef_Field(m, i);
        const char* f_name = upb_FieldDef_Name(f);
        if (strlen(f_name) == len && strncmp(f_name, name, len) == 0) {
            return f;
        }
    }
    return NULL;
}

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

#include "xs/protobuf/registry.h"

uint64_t PerlUpb_MessageDef_GetFingerprint(pTHX_ const upb_MessageDef *m) {
    if (!m) return 0;
    const char* full_name = upb_MessageDef_FullName(m);

    // Simple stable hash (FNV-1a 64-bit)
    uint64_t hash = 0xcbf29ce484222325ULL;
    const char* p = full_name;
    while (*p) {
        hash ^= (uint64_t)(unsigned char)*p++;
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

void PerlUpb_MessageDef_RegisterFingerprint(pTHX_ const upb_MessageDef *m) {
    if (!m) return;
    uint64_t fingerprint = PerlUpb_MessageDef_GetFingerprint(aTHX_ m);
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);

    if (!reg->descriptor_fingerprints) {
        croak("descriptor_fingerprints HV is NULL in registry");
    }

    char key[32];
    snprintf(key, sizeof(key), "%" PRIu64, fingerprint);

    SV* wrapper = PerlUpb_MessageDef_GetWrapper(aTHX_ m);
    if (!wrapper || !SvOK(wrapper)) {
        croak("Failed to get wrapper for MessageDef during fingerprint registration");
    }

    // Increment refcount because hv_store takes ownership but we want to keep the wrapper alive in cache too
    SvREFCNT_inc(wrapper);
    if (!hv_store(reg->descriptor_fingerprints, key, strlen(key), wrapper, 0)) {
        SvREFCNT_dec(wrapper);
        croak("hv_store failed in RegisterFingerprint");
    }
}

const upb_MessageDef* PerlUpb_MessageDef_FindByFingerprint(pTHX_ uint64_t fingerprint) {
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    char key[21];
    snprintf(key, sizeof(key), "%" PRIu64, fingerprint);

    SV** svp = hv_fetch(reg->descriptor_fingerprints, key, strlen(key), 0);
    if (svp && SvROK(*svp)) {
        return PerlUpb_MessageDef_GetMessage(aTHX_ *svp);
    }
    return NULL;
}

SV* PerlUpb_MessageDef_GetWrapper(pTHX_ const upb_MessageDef *m) {
    RETURN_CACHED_OR_CREATE_BLESSED(m, "Protobuf::Descriptor::MessageDef");
}

const upb_MessageDef* PerlUpb_MessageDef_GetMessage(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_MessageDef, sv, "Protobuf::Descriptor::MessageDef");
}

SV* PerlUpb_Message_FullName(pTHX_ const upb_MessageDef *m) {
    if (!m) return newSV(0);
    const char* full_name = upb_MessageDef_FullName(m);
    return full_name ? newSVpv(full_name, 0) : newSV(0);
}

SV* PerlUpb_Message_File(pTHX_ const upb_MessageDef *m) {
    if (!m) return &PL_sv_undef;
    const upb_FileDef* f = upb_MessageDef_File(m);
    return PerlUpb_FileDef_GetWrapper(aTHX_ f);
}

SV* PerlUpb_MessageDef_PerlClassName(pTHX_ const upb_MessageDef* mdef) {
    if (!mdef) return &PL_sv_undef;
    HV* stash = PerlUpb_GetMessageStash(aTHX_ mdef);
    return newSVpv(HvNAME(stash), 0);
}



void PerlUpb_MessageDef_AuditIdentity(pTHX_ SV* self) {
    const upb_MessageDef* m = PerlUpb_MessageDef_GetMessage(aTHX_ self);
    if (m) {
        PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_HIT, m);
    }
}
