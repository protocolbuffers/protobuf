#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_pool/add.h"
#include "xs/descriptor_pool/pool.h"
#include "xs/descriptor/file.h"
#include "upb/reflection/def.h"

#ifdef GOOGLE3
#include "google/protobuf/descriptor.upb.h"
#else
#include "google/protobuf/descriptor.upb.h"
#endif


SV* PerlUpb_DescriptorPool_AddSerializedFile(pTHX_ SV* self, SV* serialized) {
    if (PerlUpb_DescriptorPool_IsFrozen(aTHX_ self)) {
        croak("Cannot add file to a frozen DescriptorPool");
    }
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;

    STRLEN len;
    const char* data = SvPVbyte(serialized, len);

    upb_Arena* arena = PerlUpb_Arena_Acquire(aTHX_ PERL_UPB_LIFECYCLE_TRANSIENT);
    google_protobuf_FileDescriptorProto* proto = google_protobuf_FileDescriptorProto_parse(data, len, arena);
    if (!proto) {
        PerlUpb_Arena_Release(aTHX_ arena, PERL_UPB_LIFECYCLE_TRANSIENT);
        croak("Failed to parse FileDescriptorProto");
    }

    upb_Status status;
    upb_Status_Clear(&status);
    const upb_FileDef* file = upb_DefPool_AddFile((upb_DefPool*)pool, (const void*)proto, &status);

    // The pool keeps its own internal state, so the proto is no longer needed.
    PerlUpb_Arena_Release(aTHX_ arena, PERL_UPB_LIFECYCLE_TRANSIENT);

    if (!file) {
        const char* msg = upb_Status_ErrorMessage(&status);
        if (strstr(msg, "duplicate symbol")) {
            // Try to find WHICH symbol and WHERE it is
            const char* symbol = strrchr(msg, ' ');
            if (symbol) {
                symbol++; // Skip space
                const char* existing_file_name = "unknown file";
                const char* type_str = "symbol";

                const upb_MessageDef* m = upb_DefPool_FindMessageByName(pool, symbol);
                if (m) {
                    existing_file_name = upb_FileDef_Name(upb_MessageDef_File(m));
                    type_str = "message";
                } else {
                    const upb_EnumDef* e = upb_DefPool_FindEnumByName(pool, symbol);
                    if (e) {
                        existing_file_name = upb_FileDef_Name(upb_EnumDef_File(e));
                        type_str = "enum";
                    } else {
                        const upb_ServiceDef* s = upb_DefPool_FindServiceByName(pool, symbol);
                        if (s) {
                            existing_file_name = upb_FileDef_Name(upb_ServiceDef_File(s));
                            type_str = "service";
                        }
                    }
                }
                croak("Failed to add file to pool: %s (duplicate %s '%s' already defined in %s)",
                      msg, type_str, symbol, existing_file_name);
            }
        } else if (strstr(msg, "duplicate file name")) {
            croak("Failed to add file to pool: %s (already defined in this DescriptorPool)", msg);
        }
        croak("Failed to add file to pool: %s", msg);
    }

    return PerlUpb_FileDef_GetWrapper(aTHX_ file);
}

SV* PerlUpb_DescriptorPool_AddSerializedFileDescriptorSet(pTHX_ SV* self, SV* serialized) {
    if (PerlUpb_DescriptorPool_IsFrozen(aTHX_ self)) {
        croak("Cannot add file to a frozen DescriptorPool");
    }
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;

    STRLEN len;
    const char* data = SvPVbyte(serialized, len);

    upb_Arena* arena = PerlUpb_Arena_Acquire(aTHX_ PERL_UPB_LIFECYCLE_TRANSIENT);
    google_protobuf_FileDescriptorSet* set = google_protobuf_FileDescriptorSet_parse(data, len, arena);
    if (!set) {
        PerlUpb_Arena_Release(aTHX_ arena, PERL_UPB_LIFECYCLE_TRANSIENT);
        croak("Failed to parse FileDescriptorSet");
    }

    size_t n;
    const google_protobuf_FileDescriptorProto* const* files = google_protobuf_FileDescriptorSet_file(set, &n);

    upb_Status status;
    AV* av = newAV();

    for (size_t i = 0; i < n; i++) {
        upb_Status_Clear(&status);
        const upb_FileDef* file = upb_DefPool_AddFile((upb_DefPool*)pool, (const void*)files[i], &status);
        if (!file) {
            const char* msg = upb_Status_ErrorMessage(&status);
            PerlUpb_Arena_Release(aTHX_ arena, PERL_UPB_LIFECYCLE_TRANSIENT);
            SvREFCNT_dec(av);
            if (strstr(msg, "duplicate symbol")) {
                const char* symbol = strrchr(msg, ' ');
                if (symbol) {
                    symbol++;
                    const char* existing_file_name = "unknown file";
                    const char* type_str = "symbol";

                    const upb_MessageDef* m = upb_DefPool_FindMessageByName(pool, symbol);
                    if (m) {
                        existing_file_name = upb_FileDef_Name(upb_MessageDef_File(m));
                        type_str = "message";
                    } else {
                        const upb_EnumDef* e = upb_DefPool_FindEnumByName(pool, symbol);
                        if (e) {
                            existing_file_name = upb_FileDef_Name(upb_EnumDef_File(e));
                            type_str = "enum";
                        } else {
                            const upb_ServiceDef* s = upb_DefPool_FindServiceByName(pool, symbol);
                            if (s) {
                                existing_file_name = upb_FileDef_Name(upb_ServiceDef_File(s));
                                type_str = "service";
                            }
                        }
                    }
                    croak("Failed to add file %zu to pool: %s (duplicate %s '%s' already defined in %s)",
                          i, msg, type_str, symbol, existing_file_name);
                }
            } else if (strstr(msg, "duplicate file name")) {
                croak("Failed to add file %zu to pool: %s (already defined in this DescriptorPool)", i, msg);
            }
            croak("Failed to add file %zu to pool: %s", i, msg);
        }
        av_push(av, PerlUpb_FileDef_GetWrapper(aTHX_ file));
    }

    PerlUpb_Arena_Release(aTHX_ arena, PERL_UPB_LIFECYCLE_TRANSIENT);
    return newRV_noinc((SV*)av);
}
// Trivial newline to force rebuild

