#include "t/c/upb-perl-test.h"
#include "xs/protobuf/arena.h"
#include <stdlib.h>

void* test_thread_func(void* arg) {
    // Just a dummy func
    return NULL;
}

int main(int argc, char** argv) {
    plan(6);

    upb_Arena* arena = upb_Arena_New();

    LEAK_CHECK(arena, {
        void* p = upb_Arena_Malloc(arena, 100);
        (void)p;
    }, "Should FAIL leak check (expectedly)");

    LEAK_CHECK(arena, {
        // no allocation
    }, "Should PASS leak check");

    void* thread_args[4] = {NULL, NULL, NULL, NULL};
    STRESS_THREADS(4, test_thread_func, thread_args);

    TODO {
        ok(0, "System handled randomized allocation failures gracefully");
    }

    TODO {
        ok(0, "Vectorized validation paths fully exercised and reported");
    }

    TODO {
        ok(0, "Serialization output matches canonical golden binaries exactly");
    }

    upb_Arena_Free(arena);
    return 0;
}
