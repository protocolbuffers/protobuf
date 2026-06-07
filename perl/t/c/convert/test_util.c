#include "test_util.h"
#ifdef GOOGLE3
#include "google/protobuf/descriptor.upb.h"
#else
#include "google/protobuf/descriptor.upb.h"
#endif

#include "upb/base/string_view.h"
#include <stdio.h>

upb_DefPool *test_pool = NULL;

bool load_test_descriptors(pTHX_ upb_Arena *arena) {

    FILE *f = fopen("t/data/test_descriptor.bin", "rb");
    if (!f) {
        // Try Google3/Bazel sandbox path
        f = fopen("third_party/protobuf/perl/t/data/test_descriptor.bin", "rb");
    }
    if (!f) {
        fprintf(stderr, "DEBUG: load_test_descriptors fopen FAILED\n"); fflush(stderr);
        perror("Failed to open t/data/test_descriptor.bin");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char *)upb_Arena_Malloc(arena, len);
    if (!data) { cdiag("Failed upb_Arena_Malloc"); fclose(f); return false; }
    if (fread(data, 1, len, f) != len) {
        cdiag("Failed fread");
        fclose(f);
        return false;
    }
    fclose(f);

    google_protobuf_FileDescriptorSet *set = google_protobuf_FileDescriptorSet_parse(data, len, arena);
    if (!set) {
        cdiag("Failed to parse FileDescriptorSet");
        return false;
    }

    if (test_pool) { upb_DefPool_Free(test_pool); }
    test_pool = upb_DefPool_New();
    if (!test_pool) { cdiag("Failed upb_DefPool_New"); return false; }

    upb_Status status;
    size_t n;
    const google_protobuf_FileDescriptorProto *const * files = google_protobuf_FileDescriptorSet_file(set, &n);
    for (size_t i = 0; i < n; i++) {
        upb_Status_Clear(&status);
        // upb_StringView file_name = google_protobuf_FileDescriptorProto_name(files[i]);
        if (!upb_DefPool_AddFile(test_pool, files[i], &status)) {
            cdiag("Failed to add file to DefPool: %s", upb_Status_ErrorMessage(&status));
            return false;
        }
    }

    const upb_MessageDef *mdef1 = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    cdiag("protobuf_perl_test.TestMessage found: %s", mdef1 ? "YES" : "NO");

    const upb_MessageDef *mdef2 = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.NestedMessage");
    cdiag("protobuf_perl_test.NestedMessage found: %s", mdef2 ? "YES" : "NO");

    const upb_MessageDef *mdef3 = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    cdiag("protobuf_test_messages.google.protobuf.TestAllTypesProto2 found: %s", mdef3 ? "YES" : "NO");

    const upb_MessageDef *mdef4 = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2.Data");
    cdiag("protobuf_test_messages.google.protobuf.TestAllTypesProto2.Data found: %s", mdef4 ? "YES" : "NO");

    if (!mdef1 || !mdef2 || !mdef3 || !mdef4) return false;
    return true;
}

const upb_FieldDef* get_field_def(const char *msg_name, const char *field_name) {
    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, msg_name);
    if (!mdef) {
        cdiag("MessageDef %s not found", msg_name);
        return NULL;
    }
    const upb_FieldDef *fdef = upb_MessageDef_FindFieldByName(mdef, field_name);
    if (!fdef) {
        cdiag("FieldDef %s not found in %s", field_name, msg_name);
    }
    return fdef;
}
