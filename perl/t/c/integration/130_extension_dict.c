#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/extension_dict/dict.h"
#include "xs/extension_dict/iterator.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "upb/reflection/message.h"
#include <stdio.h>

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(17);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) {
         fail("Failed to load test descriptors");
         return 1;
    }
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    const upb_FieldDef *ext_field = upb_DefPool_FindExtensionByName(test_pool, "protobuf_perl_test.extension_string");

    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);
    SV* message_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);

    SV* dict_sv = PerlUpb_ExtensionDict_New(aTHX_ message_sv);
    ok(dict_sv != NULL, "Created ExtensionDict");

    // 1. Initial Get (should be default/empty)
    SV* ext_field_sv = PerlUpb_FieldDef_GetWrapper(aTHX_ ext_field);
    SV* val0 = PerlUpb_ExtensionDict_GetItem(aTHX_ dict_sv, ext_field_sv);
    is_string(SvPV_nolen(val0), "", "Default value for extension_string is empty string");
    SvREFCNT_dec(val0);

    // 2. Set and Get
    SV* val1_sv = newSVpv("val 1", 0);
    PerlUpb_ExtensionDict_SetItem(aTHX_ dict_sv, ext_field_sv, val1_sv);
    SV* val1_ret = PerlUpb_ExtensionDict_GetItem(aTHX_ dict_sv, ext_field_sv);
    is_string(SvPV_nolen(val1_ret), "val 1", "Set/Get roundtrip matches");
    SvREFCNT_dec(val1_ret);

    // 3. Overwrite
    SV* val2_sv = newSVpv("val 2", 0);
    PerlUpb_ExtensionDict_SetItem(aTHX_ dict_sv, ext_field_sv, val2_sv);
    SV* val2_ret = PerlUpb_ExtensionDict_GetItem(aTHX_ dict_sv, ext_field_sv);
    is_string(SvPV_nolen(val2_ret), "val 2", "Overwrite matches");
    SvREFCNT_dec(val2_ret);

    // 4. Iterator
    SV* iter_sv = PerlUpb_ExtensionDict_GetIterator(aTHX_ dict_sv);
    int count = 0;
    SV* next_f;
    while (SvOK(next_f = PerlUpb_ExtensionDict_Iterator_Next(aTHX_ iter_sv))) {
        count++;
        const upb_FieldDef* f_raw = PerlUpb_FieldDef_GetField(aTHX_ next_f);
        ok(upb_FieldDef_IsExtension(f_raw), "Iterator returned an extension");
        is(f_raw, ext_field, "It's the expected extension");
        SvREFCNT_dec(next_f);
    }
    is(count, 1, "Correct number of extensions iterated");
    SvREFCNT_dec(next_f); // Final undef

    extern void PerlUpb_ExtensionDictIterator_Free(pTHX_ SV* sv);
    PerlUpb_ExtensionDictIterator_Free(aTHX_ iter_sv);
    SvREFCNT_dec(iter_sv);

    // 5. Verify field properties
    ok(upb_FieldDef_IsExtension(ext_field), "Verify IsExtension true");
    const upb_FieldDef* normal_field = upb_MessageDef_FindFieldByNumber(mdef, 1);
    ok(!upb_FieldDef_IsExtension(normal_field), "Verify IsExtension false for normal field");

    ok(1, "Croak test for normal field (skipped in C)");

    TODO {
        ok(0, "Deep-copy logic for extensions between messages on different arenas verified");
    }

    TODO {
        ok(0, "Trace logging for extension inflation and discard events");
    }

    TODO {
        ok(0, "Wrappers remain valid or are safely invalidated when parent message is re-parsed");
    }

    TODO {
        ok(0, "Extension descriptors are resolved and cached during the main parse pass");
    }

    TODO {
        ok(0, "System detects and reports when reified extensions are incompatible with new pool versions");
    }

    TODO {
        ok(0, "Retrieve all reified extensions in a single XS call to minimize conversion overhead");
    }

    // Cleanup
    SvREFCNT_dec(ext_field_sv);
    SvREFCNT_dec(val1_sv);
    SvREFCNT_dec(val2_sv);

    extern void PerlUpb_ExtensionDict_Free(pTHX_ SV* sv);
    PerlUpb_ExtensionDict_Free(aTHX_ dict_sv);
    SvREFCNT_dec(dict_sv);

    SvREFCNT_dec(message_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
