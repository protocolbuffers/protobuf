#include "t/c/convert/types/group.h"
#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "upb/reflection/def.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include <stdint.h>
#include <string.h>

// test_num is external from the main runner
extern int test_num;

// upb_to_sv
static upb_Message* create_optionalgroup_upb(upb_Arena *arena, int32_t val) {
    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2.Data");
    if (!mdef) {
        fprintf(stderr, "ERROR: create_optionalgroup_upb - mdef is NULL for TestAllTypesProto2.Data\n");
        return NULL;
    }
    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);
    const upb_FieldDef *fdef = upb_MessageDef_FindFieldByName(mdef, "group_int32");
    if (!fdef) {
        fprintf(stderr, "ERROR: create_optionalgroup_upb - fdef is NULL for group_int32\n");
        return NULL;
    }
    const upb_MiniTableField *mfield = upb_FieldDef_MiniTable(fdef);
    upb_MessageValue msg_val;
    msg_val.int32_val = val;
    upb_Message_SetBaseField(msg, mfield, &msg_val);
    return msg;
}

static void set_optionalgroup_val(upb_MessageValue *val, upb_Arena *arena) {
    val->msg_val = create_optionalgroup_upb(arena, 17);
}

static void check_sv_optionalgroup(pTHX_ SV *sv, const char *prefix) {
    ok(SvROK(sv), sdiagnostic("%s: SV is a reference", prefix));
    if (!SvROK(sv)) return;
    SV *deref = SvRV(sv);
    ok(SvTYPE(deref) == SVt_PVHV, sdiagnostic("%s: Dereferenced SV is a HASH", prefix));
    // Further checks would require Perl-level object interaction
}

const upb_to_sv_test_case group_upb_to_sv_test_cases[] = {
    {"data", "group", kUpb_FieldType_Group, set_optionalgroup_val, check_sv_optionalgroup, 2},
    {NULL}
};

// sv_to_upb
const sv_to_upb_test_case group_sv_to_upb_test_cases[] = {
    // SvToUpb for groups/messages is complex, test at Perl level
    {NULL}
};
