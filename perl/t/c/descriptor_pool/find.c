#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor_pool/pool.h"
#include "xs/descriptor_pool/add.h"
#include "xs/descriptor_pool/find.h"
#include <string.h>

// Minimal serialized FileDescriptorProto: name = "find_test.proto", message = "Test"
unsigned char find_test_proto[] = {
    0x0a, 0x0f, 'f', 'i', 'n', 'd', '_', 't', 'e', 's', 't', '.', 'p', 'r', 'o', 't', 'o',
    0x22, 0x06, 0x0a, 0x04, 'T', 'e', 's', 't'
};

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(6);

    SV* pool_sv = PerlUpb_DescriptorPool_New(aTHX);
    SV* serialized = newSVpvn((const char*)find_test_proto, sizeof(find_test_proto));
    SV* file_sv = PerlUpb_DescriptorPool_AddSerializedFile(aTHX_ pool_sv, serialized);

    // 1. FindMessageByName
    SV* msg_sv = PerlUpb_DescriptorPool_FindMessageByName(aTHX_ pool_sv, "Test");
    ok(sv_derived_from(msg_sv, "Protobuf::Descriptor::MessageDef"), "FindMessageByName finds message 'Test'");

    // 2. FindFileByName
    SV* file_sv_found = PerlUpb_DescriptorPool_FindFileByName(aTHX_ pool_sv, "find_test.proto");
    ok(sv_derived_from(file_sv_found, "Protobuf::Descriptor::File"), "FindFileByName finds file 'find_test.proto'");

    // 3. Find non-existent
    SV* none = PerlUpb_DescriptorPool_FindMessageByName(aTHX_ pool_sv, "NoSuchThing");
    ok(!SvOK(none), "Find returns undef for non-existent item");

    TODO lookup") {
        ok(0, "Implement detailed conflict error metadata");
        ok(0, "Add source-file and line-number tracking to conflicts");
        ok(0, "Implement O(1) Descriptor lookup by fingerprinted hash");
    }

    SvREFCNT_dec(none);
    SvREFCNT_dec(file_sv_found);
    SvREFCNT_dec(msg_sv);
    SvREFCNT_dec(file_sv);
    SvREFCNT_dec(serialized);
    SvREFCNT_dec(pool_sv);

    test_perl_destroy(my_perl);
    return 0;
}
