#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_HHVM_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_HHVM_H__

#include "protobuf_cpp.h"

extern Object internal_generated_pool;

const upb_msgdef *class2msgdef(const Class *klass);
const Class *msgdef2class(const upb_msgdef *msgdef);

upb_msgval tomsgval(const Variant& value, upb_fieldtype_t type);
Variant tophpval(const upb_msgval& msgval,
                 upb_fieldtype_t type,
                 Class* klass);

extern const StaticString s_RepeatedField;
extern const StaticString s_MapField;

#endif // __GOOGLE_PROTOBUF_PHP_PROTOBUF_HHVM_H__
