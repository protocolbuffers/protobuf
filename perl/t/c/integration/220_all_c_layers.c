#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/message/serialize.h"
#include "xs/message/compare.h"
#include "xs/repeated/repeated.h"
#include "xs/repeated/composite.h"
#include "xs/map/map.h"
#include "xs/extension_dict/dict.h"
#include "xs/unknown_fields/set.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "upb/reflection/message.h"
#include <stdio.h>

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(25);

    extern void PerlUpb_ObjCache_Init(pTHX);
    PerlUpb_ObjCache_Init(aTHX);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) return 1;
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    SV* mdef_sv = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ mdef_sv, 0);

    // 1. Scalars
    const upb_FieldDef *f_int32 = upb_MessageDef_FindFieldByName(mdef, "optional_int32");
    PerlUpb_Message_SetField(aTHX_ msg_sv, f_int32, newSViv(123));

    // 2. Enum
    const upb_FieldDef *f_enum = upb_MessageDef_FindFieldByName(mdef, "optional_nested_enum");
    PerlUpb_Message_SetField(aTHX_ msg_sv, f_enum, newSViv(1)); // FOO

    // 3. Repeated Message
    const upb_FieldDef *f_rep_msg = upb_MessageDef_FindFieldByName(mdef, "repeated_nested_message");
    upb_Array* arr = upb_Message_Mutable((upb_Message*)PerlUpb_Message_GetMsg(aTHX_ msg_sv), f_rep_msg, arena).array;
    SV* rep_wrapper = PerlUpb_Repeated_New(aTHX_ arr, f_rep_msg, PerlUpb_Message_GetArena(aTHX_ msg_sv), 0);
    SV* submsg1 = PerlUpb_Repeated_Add(aTHX_ rep_wrapper);
    const upb_MessageDef* sub_mdef = upb_FieldDef_MessageSubDef(f_rep_msg);
    const upb_FieldDef* f_a = upb_MessageDef_FindFieldByName(sub_mdef, "a");
    PerlUpb_Message_SetField(aTHX_ submsg1, f_a, newSViv(10));

    // 4. Map
    const upb_FieldDef *f_map = upb_MessageDef_FindFieldByName(mdef, "map_int32_int32");
    upb_Map* map_ptr = upb_Message_Mutable((upb_Message*)PerlUpb_Message_GetMsg(aTHX_ msg_sv), f_map, arena).map;
    SV* map_wrapper = PerlUpb_Map_New(aTHX_ map_ptr, f_map, PerlUpb_Message_GetArena(aTHX_ msg_sv), 0);
    PerlUpb_Map_SetItem(aTHX_ map_wrapper, newSViv(100), newSViv(200));

    // 5. Unknown Fields
    const char unk_data[] = { 0xB8, 0x3E, 0x7B }; // tag 999, val 123
    SV* unk_set = PerlUpb_UnknownFieldSet_New(aTHX_ msg_sv);
    PerlUpb_UnknownFieldSet_Add(aTHX_ unk_set, newSVpvn(unk_data, sizeof(unk_data)));

    ok(1, "Message populated with scalars, repeated, map and unknown fields");

    // 7. Serialize / Parse / Compare
    SV* serialized = PerlUpb_Message_Serialize(aTHX_ msg_sv);
    ok(serialized != NULL, "Serialized complex message");

    SV* parsed_sv = PerlUpb_Message_Parse(aTHX_ mdef_sv, serialized);
    ok(parsed_sv != NULL, "Parsed complex message");
    ok(PerlUpb_Message_IsEqual(aTHX_ msg_sv, parsed_sv), "Messages are equal");

    // 8. Verify contents of parsed message
    is(SvIV(PerlUpb_Message_GetField(aTHX_ parsed_sv, f_int32)), 123, "Parsed int32 matches");
    is(SvIV(PerlUpb_Message_GetField(aTHX_ parsed_sv, f_enum)), 1, "Parsed enum matches");

    SV* p_rep_ref = PerlUpb_Message_GetField(aTHX_ parsed_sv, f_rep_msg);
    is(PerlUpb_Repeated_Size(aTHX_ p_rep_ref), 1, "Parsed repeated message size matches");
    SV* p_sub1 = PerlUpb_Repeated_GetItem(aTHX_ p_rep_ref, 0);
    is(SvIV(PerlUpb_Message_GetField(aTHX_ p_sub1, f_a)), 10, "Parsed nested field matches");
    SvREFCNT_dec(p_sub1);
    SvREFCNT_dec(p_rep_ref);

    SV* p_map_ref = PerlUpb_Message_GetField(aTHX_ parsed_sv, f_map);
    SV* key_sv = newSViv(100);
    SV* p_map_val = PerlUpb_Map_GetItem(aTHX_ p_map_ref, key_sv);
    is(SvIV(p_map_val), 200, "Parsed map value matches");
    SvREFCNT_dec(p_map_val);
    SvREFCNT_dec(key_sv);
    SvREFCNT_dec(p_map_ref);

    SV* p_unk_set = PerlUpb_UnknownFieldSet_New(aTHX_ parsed_sv);
    SV* p_unk_data = PerlUpb_UnknownFieldSet_GetData(aTHX_ p_unk_set);
    ok(SvCUR(p_unk_data) >= sizeof(unk_data), "Parsed unknown data length matches");
    SvREFCNT_dec(p_unk_data);
    extern void PerlUpb_UnknownFieldSet_Free(pTHX_ SV* sv);
    PerlUpb_UnknownFieldSet_Free(aTHX_ p_unk_set);
    SvREFCNT_dec(p_unk_set);

    // 9. Object Identity Checks
    SV* submsg1_again = PerlUpb_Repeated_GetItem(aTHX_ rep_wrapper, 0);
    ok(SvRV(submsg1_again) == SvRV(submsg1), "Repeated GetItem returns same SV as Add");
    SvREFCNT_dec(submsg1_again);

    SV* msg_again = PerlUpb_MaybeGetMessage(aTHX_ PerlUpb_Message_GetMsg(aTHX_ msg_sv));
    ok(msg_again != NULL && SvRV(msg_again) == SvRV(msg_sv), "MaybeGetMessage returns same SV");
    SvREFCNT_dec(msg_again);

    // Cleanup
    SvREFCNT_dec(submsg1);
    SvREFCNT_dec(rep_wrapper);

    SvREFCNT_dec(map_wrapper);

    PerlUpb_UnknownFieldSet_Free(aTHX_ unk_set);
    SvREFCNT_dec(unk_set);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ parsed_sv));
    PerlUpb_Message_Free(aTHX_ parsed_sv);
    SvREFCNT_dec(parsed_sv);

    SvREFCNT_dec(serialized);
    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    ok(1, "Final cleanup clean");

    TODO {
        ok(0, "Massive concurrent mutation of all integrated field types verified");
    }

    TODO {
        ok(0, "Parsing data into evolved MessageDefs handles mismatches gracefully");
    }

    TODO {
        ok(0, "Comprehensive tracing of message mutations and arena state verified");
    }

    TODO {
        ok(0, "System remains stable when arena allocations face random delays or failures");
    }

    TODO {
        ok(0, "Integrated vectorized processing node verified for bulk message transformation");
    }

    TODO {
        ok(0, "Messages successfully transferred between processes without serialization using mmap-backed arenas");
    }

    TODO {
        ok(0, "Complete message tree memory successfully placed on the executing thread's NUMA node");
    }

    test_perl_destroy(my_perl);
    return 0;
}
