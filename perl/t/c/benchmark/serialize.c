#include <stdio.h>
#include <time.h>
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/wire/encode.h"
#include "upb/reflection/def.h"
#ifdef GOOGLE3
#include "google/protobuf/descriptor.upb.h"
#define google_protobuf_FileDescriptorSet proto2_FileDescriptorSet
#define google_protobuf_FileDescriptorSet_parse proto2_FileDescriptorSet_parse
#define google_protobuf_FileDescriptorProto proto2_FileDescriptorProto
#define google_protobuf_FileDescriptorSet_file proto2_FileDescriptorSet_file
#define google_protobuf_FileDescriptorProto_name proto2_FileDescriptorProto_name
#else
#include "google/protobuf/descriptor.upb.h"
#endif
#include "../upb-perl-test.h"

#define ITERATIONS 1000000

// Globals for simplicity in benchmark
upb_DefPool *test_pool = NULL;

bool load_benchmark_descriptors(upb_Arena *arena) {
    FILE *f = fopen("t/data/test_descriptor.bin", "rb");
    if (!f) {
        // Try Google3/Bazel sandbox path
        f = fopen("third_party/protobuf/perl/t/data/test_descriptor.bin", "rb");
    }
    if (!f) {
        perror("Failed to open t/data/test_descriptor.bin");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char *)upb_Arena_Malloc(arena, len);
    if (fread(data, 1, len, f) != len) {
        fclose(f);
        return false;
    }
    fclose(f);

    google_protobuf_FileDescriptorSet *set = google_protobuf_FileDescriptorSet_parse(data, len, arena);
    if (!set) return false;

    test_pool = upb_DefPool_New();
    upb_Status status;
    size_t n;
    const google_protobuf_FileDescriptorProto *const * files = google_protobuf_FileDescriptorSet_file(set, &n);
    for (size_t i = 0; i < n; i++) {
        upb_Status_Clear(&status);
        if (!upb_DefPool_AddFile(test_pool, files[i], &status)) {
            fprintf(stderr, "Failed to add file to DefPool: %s\n", upb_Status_ErrorMessage(&status));
            return false;
        }
    }
    return true;
}

int main() {
    upb_Arena *arena = upb_Arena_New();

    if (!load_benchmark_descriptors(arena)) {
        fprintf(stderr, "Failed to load descriptors\n");
        return 1;
    }

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    if (!mdef) {
        fprintf(stderr, "Failed to find message definition\n");
        return 1;
    }

    const upb_MiniTable *mini_table = upb_MessageDef_MiniTable(mdef);
    upb_Message *msg = upb_Message_New(mini_table, arena);

    ASSERT_ARENA_CLEAN(arena, "Arena exists and is ready for benchmark");

    // Initial setup
    clock_t start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        char *buf;
        size_t size;
        upb_EncodeStatus status = upb_Encode(msg, mini_table, 0, arena, &buf, &size);
        if (status != kUpb_EncodeStatus_Ok) {
            fprintf(stderr, "Encoding failed with status %d\n", status);
            return 1;
        }
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Serialization Benchmark: %d iterations in %.4f seconds (%.2f ops/sec)\n",
           ITERATIONS, elapsed, ITERATIONS / elapsed);

    ok(elapsed > 0, "Benchmark completed in positive time");

    upb_DefPool_Free(test_pool);
    upb_Arena_Free(arena);
    return 0;
}
