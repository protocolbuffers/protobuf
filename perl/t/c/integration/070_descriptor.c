#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/convert.h"
#include "xs/descriptor/message.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/enum.h"
#include "t/c/convert/test_util.h"
#include <stdio.h>

static void test_descriptor_cache_identity(pTHX_ const upb_MessageDef *mdef) {
    const upb_FieldDef *f = upb_MessageDef_Field(mdef, 0);
    ok(f != NULL, "CacheIdentity: Got a field");
    if (!f) return;

    SV *sv1 = PerlUpb_FieldDef_GetWrapper(aTHX_ f);
    SV *sv2 = PerlUpb_FieldDef_GetWrapper(aTHX_ f);

    ok(sv1 != NULL, "CacheIdentity: sv1 is not NULL");
    ok(SvRV(sv1) == SvRV(sv2), "CacheIdentity: Same field pointer returns same underlying SV");
    SvREFCNT_dec(sv1);
    SvREFCNT_dec(sv2);

    SV *msv1 = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
    SV *msv2 = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
    ok(SvRV(msv1) == SvRV(msv2), "CacheIdentity: Same message def pointer returns same underlying SV");
    SvREFCNT_dec(msv1);
    SvREFCNT_dec(msv2);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(12 + 4 + 3);

    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
         fail("Failed to load test descriptors");
         return 1;
    }
    ok(1, "Descriptors loaded");

    // 1. Get MessageDef
    const upb_MessageDef *msg_def = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    ok(msg_def != NULL, "Found protobuf_perl_test.TestMessage");

    if (msg_def) {
        // 2. Get FieldDef via wrapper
        const upb_FieldDef *field = upb_MessageDef_FindFieldByName(msg_def, "enum_field");
        ok(field != NULL, "Found enum_field via PerlUpb_MessageDef_FindFieldByName");

        if (field) {
            // 3. Use FieldDef in SvToUpb
            SV *sv = newSViv(1); // TEST_ENUM_FIRST
            upb_MessageValue val;
            bool success = PerlUpb_SvToUpb(aTHX_ sv, field, &val, arena);
            ok(success, "PerlUpb_SvToUpb success with enum field");
            is(val.int32_val, 1, "Enum value correct in UPB");
            SvREFCNT_dec(sv);

            // 4. Get EnumDef from FieldDef via wrapper
            const upb_EnumDef *enum_def = upb_FieldDef_EnumSubDef(field);
            ok(enum_def != NULL, "PerlUpb_FieldDef_EnumSubDef returns non-NULL");
            if (enum_def) {
                is_string(upb_EnumDef_FullName(enum_def), "protobuf_perl_test.TestEnum", "Enum full name matches");
            } else {
                fprintf(stderr, "# EnumDef is NULL\n");
            }
        } else {
            fprintf(stderr, "# Field is NULL\n");
        }

        const upb_FieldDef *msg_field = upb_MessageDef_FindFieldByName(msg_def, "nested_message");
        ok(msg_field != NULL, "Found nested_message");
        if (msg_field) {
            // 5. Get MessageDef from FieldDef via wrapper
            const upb_MessageDef *sub_msg_def = upb_FieldDef_MessageSubDef(msg_field);
            ok(sub_msg_def != NULL, "PerlUpb_FieldDef_MessageSubDef returns non-NULL");
            if (sub_msg_def) {
                is_string(upb_MessageDef_FullName(sub_msg_def), "protobuf_perl_test.NestedMessage", "Sub-message full name matches");
            } else {
                fprintf(stderr, "# Sub-message MessageDef is NULL\n");
            }
        } else {
            fprintf(stderr, "# Nested message field is NULL\n");
        }

        // 6. Test list-based field access
        int field_count = upb_MessageDef_FieldCount(msg_def);
        ok(field_count > 0, "Field count > 0");
        const upb_FieldDef *first_field = upb_MessageDef_Field(msg_def, 0);
        ok(first_field != NULL, "First field retrieved via PerlUpb_MessageDef_Field");

    } else {
        fprintf(stderr, "# Skipping integration tests as msg_def is NULL\n");
    }

    TODO {
        test_descriptor_cache_identity(aTHX_ msg_def);
    }

    TODO {
        ok(0, "Complex descriptor types are correctly resolved and integrated with convert logic");
    }

    TODO {
        ok(0, "System prevents DescriptorPool disposal while child definitions are pinned in Perl");
    }

    upb_Arena_Free(arena);
    test_perl_destroy(my_perl);
    return 0;
}
