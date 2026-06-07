#ifndef PERL_PROTOBUF_DESCRIPTOR_BASE_H_
#define PERL_PROTOBUF_DESCRIPTOR_BASE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"
#include "xs/protobuf/utils.h"

// Common functions for all descriptor types
#define RETURN_CACHED_OR_CREATE_BLESSED(ptr, class_name)       \
  return PerlUpb_WrapArenaBoundObject(aTHX_(void*)(ptr), NULL, \
                                      gv_stashpv(class_name, GV_ADD), 0)

#define EXTRACT_CACHED_DESCRIPTOR(ptr_type, sv, class_name) \
  return (const ptr_type*)PerlUpb_GetArenaBoundObject(aTHX_ sv, class_name)

// XS macros to reduce boilerplate in .xs files
#define XS_STR_ACCESSOR_BODY(ptr_type, get_ptr, upb_func) \
  const ptr_type* ptr = get_ptr(aTHX_ self);              \
  RETVAL = ptr ? upb_func(ptr) : NULL;

#define XS_INT_ACCESSOR_BODY(ptr_type, get_ptr, upb_func, default_val) \
  const ptr_type* ptr = get_ptr(aTHX_ self);                           \
  RETVAL = ptr ? upb_func(ptr) : default_val;

#define XS_BOOL_ACCESSOR_BODY(ptr_type, get_ptr, upb_func) \
  const ptr_type* ptr = get_ptr(aTHX_ self);               \
  RETVAL = ptr ? upb_func(ptr) : false;

#endif  // PERL_PROTOBUF_DESCRIPTOR_BASE_H_
