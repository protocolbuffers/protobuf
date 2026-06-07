#include "t/c/upb-perl-test.h"

int main(int argc, char** argv) {
    plan(4);

    SKIP("test simple skip", 1);

    SKIP_BLOCK(1, 2, "test skip block") {
        ok(0, "should be skipped 1");
        ok(0, "should be skipped 2");
    }

    SKIP_BLOCK(0, 1, "test no skip block") {
        ok(1, "should NOT be skipped");
    }

    return 0;
}
