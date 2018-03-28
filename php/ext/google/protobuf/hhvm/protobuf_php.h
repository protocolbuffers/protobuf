#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__

#include "protobuf_cpp.h"

#define MAX_LENGTH_OF_INT64 20
#define SIZEOF_INT64 8

#if PHP_MAJOR_VERSION < 7
extern zval* internal_generated_pool;
#else
extern zend_object *internal_generated_pool;
#endif

upb_msgval tomsgval(zval* value, upb_fieldtype_t type, upb_alloc* alloc);
void tophpval(const upb_msgval &msgval,
              upb_fieldtype_t type,
              zend_class_entry *subklass,
              ARENA arena,
              zval *retval);

void MapField_offsetSet(MapField* intern, zval* key, zval* value);
void RepeatedField_append(RepeatedField *intern, zval *value);

extern zend_class_entry* RepeatedField_type;
extern zend_class_entry* MapField_type;

// -----------------------------------------------------------------------------
// Type conversion.
// -----------------------------------------------------------------------------

bool protobuf_convert_to_int32(zval* from, int32_t* to);
bool protobuf_convert_to_uint32(zval* from, uint32_t* to);
bool protobuf_convert_to_int64(zval* from, int64_t* to);
bool protobuf_convert_to_uint64(zval* from, uint64_t* to);
bool protobuf_convert_to_float(zval* from, float* to);
bool protobuf_convert_to_double(zval* from, double* to);
bool protobuf_convert_to_bool(zval* from, int8_t* to);
bool protobuf_convert_to_string(zval* from);

#endif // __GOOGLE_PROTOBUF_PHP_PROTOBUF_PHP_H__
