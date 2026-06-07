#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor_pool/pool.h"
#include "xs/descriptor_pool/find.h"
#include "xs/descriptor/message.h"
#include "libcoro/coro.h"
#include "t/c/coro_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define NUM_COROS 50
#define NUM_OPS 100
#define STACK_SIZE 65536

DECLARE_CORO_STATE()

typedef struct {
    int id;
    int errors;
    SV* pool_sv;
} coro_arg_t;

void test_pool_access(pTHX_ coro_arg_t *carg) {
    SV* msg_sv = PerlUpb_DescriptorPool_FindMessageByName(aTHX_ carg->pool_sv, "Test");
    if (!msg_sv || !SvOK(msg_sv)) {
        carg->errors++;
        return;
    }

    if (!sv_derived_from(msg_sv, "Protobuf::Descriptor::MessageDef")) {
        carg->errors++;
    }

    SV* msg_sv_again = PerlUpb_DescriptorPool_FindMessageByName(aTHX_ carg->pool_sv, "Test");
    if (SvRV(msg_sv_again) != SvRV(msg_sv)) {
        carg->errors++;
    }

    SvREFCNT_dec(msg_sv);
    SvREFCNT_dec(msg_sv_again);
}

void coro_test_func(void *arg) {
    coro_arg_t *carg = (coro_arg_t *)arg;
    dTHX;
    for (int i = 0; i < NUM_OPS; i++) {
        test_pool_access(aTHX_ carg);
        coro_yield(carg->id);
    }
    coro_finish(carg->id);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(3 + NUM_COROS + 2);

    extern void PerlUpb_ObjCache_Init(pTHX);
    PerlUpb_ObjCache_Init(aTHX);

    SV* pool_sv = PerlUpb_DescriptorPool_GeneratedPool(aTHX);
    ok(pool_sv != NULL, "Got GeneratedPool");

    unsigned char test_proto[] = {
        0x0a, 0x0a, 't', 'e', 's', 't', '.', 'p', 'r', 'o', 't', 'o',
        0x22, 0x06, 0x0a, 0x04, 'T', 'e', 's', 't'
    };
    SV* serialized = newSVpvn((const char*)test_proto, sizeof(test_proto));
    extern SV* PerlUpb_DescriptorPool_AddSerializedFile(pTHX_ SV* self, SV* serialized);
    SV* file_sv = PerlUpb_DescriptorPool_AddSerializedFile(aTHX_ pool_sv, serialized);
    ok(file_sv != NULL, "Added test message to pool");
    SvREFCNT_dec(file_sv);
    SvREFCNT_dec(serialized);

    coro_arg_t args[NUM_COROS];
    for (int i = 0; i < NUM_COROS; i++) {
        args[i].pool_sv = pool_sv;
    }
    RUN_CORO_TEST(coro_test_func, args);

    TODO {
        ok(0, "System handles race conditions during duplicate definition addition safely");
    }

    TODO {
        ok(0, "Cache and pool internal state remain consistent during failed lookups");
    }

    SvREFCNT_dec(pool_sv);
    test_perl_destroy(my_perl);
    return 0;
}
