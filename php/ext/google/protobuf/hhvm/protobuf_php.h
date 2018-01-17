#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__

#include "protobuf_cpp.h"

#if PHP_MAJOR_VERSION < 7
extern zval* internal_generated_pool;
#else
extern zend_object *internal_generated_pool;
#endif

const upb_msgdef* class2msgdef(const zend_class_entry* klass);
const zend_class_entry* msgdef2class(const upb_msgdef* msgdef);

upb_msgval tomsgval(zval* value, upb_fieldtype_t type);
void tophpval(const upb_msgval &msgval,
              upb_fieldtype_t type,
              zend_class_entry *subklass,
              zval *retval);

extern zend_class_entry* RepeatedField_type;
extern zend_class_entry* MapField_type;

#endif // __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__
