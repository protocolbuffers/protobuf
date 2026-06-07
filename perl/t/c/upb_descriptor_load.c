#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "upb/mem/arena.h"
#include "upb/reflection/def.h"
#include "upb/base/status.h"
#include "google/protobuf/descriptor.upb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    // No Perl interpreter needed for this test

    plan(7); // Adjusted plan

    upb_Arena *arena = upb_Arena_New();
    ok(arena != NULL, "Arena created");
    if (!arena) return 1;

    FILE *f = fopen("t/data/test_descriptor.bin", "rb");
    if (!f) {
        // Try Google3/Bazel sandbox path
        f = fopen("third_party/protobuf/perl/t/data/test_descriptor.bin", "rb");
    }
    ok(f != NULL, "Opened descriptor file t/data/test_descriptor.bin");
    if (!f) {
        upb_Arena_Free(arena);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char *)malloc(len);
    if (!data) {
        fail("Failed to malloc buffer");
        fclose(f);
        upb_Arena_Free(arena);
        return 1;
    }

    ok(fread(data, 1, len, f) == len, "Read descriptor file");
    fclose(f);

    google_protobuf_FileDescriptorSet *set = google_protobuf_FileDescriptorSet_parse(data, len, arena);
    ok(set != NULL, "Parsed FileDescriptorSet");
    if (!set) {
        free(data);
        upb_Arena_Free(arena);
        return 1;
    }

    upb_DefPool *pool = upb_DefPool_New();
    ok(pool != NULL, "Created DefPool");
    if (!pool) {
        free(data);
        upb_Arena_Free(arena);
        return 1;
    }

    upb_Status status;
    upb_Status_Clear(&status);

    size_t n;
    const google_protobuf_FileDescriptorProto *const * files = google_protobuf_FileDescriptorSet_file(set, &n);
    is(n, 1, "File count in set is 1");

    bool all_ok = true;
    for (size_t i = 0; i < n; i++) {
        if (!upb_DefPool_AddFile(pool, files[i], &status)) {
            // log something here? ("Failed to add file %zu to DefPool: %s", i, upb_Status_ErrorMessage(&status));
            all_ok = false;
            break;
        }
    }
    ok(all_ok, "upb_DefPool_AddFile successful for all files");

    free(data);
    upb_DefPool_Free(pool);
    upb_Arena_Free(arena);

    return 0;
}
