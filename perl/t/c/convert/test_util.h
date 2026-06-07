#ifndef PERL_PROTOBUF_CONVERT_TEST_UTIL_H_
#define PERL_PROTOBUF_CONVERT_TEST_UTIL_H_

#include "EXTERN.h"
#include "XSUB.h"
#include "perl.h"  // NOLINT(build/include)
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/reflection/def.h"
#include "upb/wire/types.h"
#include "xs/protobuf.h"

extern upb_DefPool* test_pool;

typedef void (*set_upb_val_func_t)(upb_MessageValue* val, upb_Arena* arena);
typedef void (*check_sv_func_t)(pTHX_ SV* sv, const char* prefix);

typedef struct {
  const char* field_name;
  const char* test_prefix;
  upb_FieldType expected_type;
  set_upb_val_func_t set_val;
  check_sv_func_t check_sv;
  int num_checks;
} upb_to_sv_test_case;

typedef void (*check_upb_func_t)(const upb_MessageValue* val,
                                 const char* prefix);

typedef struct {
  const char* field_name;
  const char* test_prefix;
  upb_FieldType expected_type;
  SV* (*sv_creator)(pTHX);
  check_upb_func_t check_func;
  int num_checks;
} sv_to_upb_test_case;

bool load_test_descriptors(pTHX_ upb_Arena* arena);
const upb_FieldDef* get_field_def(const char* msg_name, const char* field_name);

#define VALID_SCALAR(elem) (SvPOK(elem) || SvIOK(elem) || SvNOK(elem))

#define GEN_CHECK_SV_REPEATED_SCALAR(type_name, expected_size)                \
  static void check_sv_repeated_##type_name##_val(pTHX_ SV* sv,               \
                                                  const char* prefix) {       \
    ok(sv_derived_from(sv, "Protobuf::Internal::Repeated"),                   \
       sdiagnostic("%s: SV is a Repeated wrapper", prefix));                  \
    if (!sv_derived_from(sv, "Protobuf::Internal::Repeated")) return;         \
    int size = PerlUpb_Repeated_Size(aTHX_ sv);                               \
    is(size, expected_size,                                                   \
       sdiagnostic("%s: Array has %d elements", prefix, expected_size));      \
    SV* elem0 = PerlUpb_Repeated_GetItem(aTHX_ sv, 0);                        \
    ok(elem0, sdiagnostic("%s: Fetched element 0", prefix));                  \
    if (elem0) {                                                              \
      ok(VALID_SCALAR(elem0), sdiagnostic("%s: Element 0 is valid", prefix)); \
      SvREFCNT_dec(elem0);                                                    \
    }                                                                         \
    SV* elem1 = PerlUpb_Repeated_GetItem(aTHX_ sv, 1);                        \
    ok(elem1, sdiagnostic("%s: Fetched element 1", prefix));                  \
    if (elem1) {                                                              \
      ok(VALID_SCALAR(elem1), sdiagnostic("%s: Element 1 is valid", prefix)); \
      SvREFCNT_dec(elem1);                                                    \
    }                                                                         \
  }

#endif  // PERL_PROTOBUF_CONVERT_TEST_UTIL_H_
