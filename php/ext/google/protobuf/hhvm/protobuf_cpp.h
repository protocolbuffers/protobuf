#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__

#include <string>
#include <map>
#include <set>

#include "port.h"
#include "upb.h"

#define PHP_PROTOBUF_EXTNAME "protobuf"
#define PHP_PROTOBUF_VERSION "3.4.1"

// -----------------------------------------------------------------------------
// Global Utils
// -----------------------------------------------------------------------------

void register_upbdef(const char* classname, const upb_def* def);

// -----------------------------------------------------------------------------
// Protobuf Module
// -----------------------------------------------------------------------------

class ProtobufModule {
 public:
  ProtobufModule();
  bool IsReservedName(const std::string& name);

 private:
  std::set<std::string> reserved_names;
};

extern ProtobufModule* protobuf_module;

// -----------------------------------------------------------------------------
// InternalDescriptorPool
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(InternalDescriptorPool);

void InternalDescriptorPool_init(TSRMLS_D);

void InternalDescriptorPool_free_c(
    InternalDescriptorPool *object TSRMLS_DC);
void InternalDescriptorPool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC);

void InternalDescriptorPool_add_generated_file(
    InternalDescriptorPool* pool, const char* data, int data_len);

PROTO_WRAP_OBJECT_START(InternalDescriptorPool)
  upb_symtab* symtab;
PROTO_WRAP_OBJECT_END

extern InternalDescriptorPool* internal_generated_pool_cpp;
extern upb_msgfactory* message_factory;

// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(Message);

void Message_init(TSRMLS_D);

void Message_free_c(Message *intern TSRMLS_DC);
void Message_init_c_instance(Message *intern TSRMLS_DC);

void Message_wrap(Message *intern, upb_msg *msg, const upb_msgdef *msgdef);

void Message___construct(Message *intern, const upb_msgdef *msgdef);
const char *Message_serializeToString(Message *intern, size_t *size);
void Message_mergeFromString(
    Message *intern, const char *data, size_t size);

// TODO(teboring): Combine all metadata.
PROTO_WRAP_OBJECT_START(Message)
  const upb_msgdef *msgdef;
  const upb_msglayout *layout;
  upb_msg *msg;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(MapField);

void MapField_init(TSRMLS_D);

void MapField_free_c(MapField *intern TSRMLS_DC);
void MapField_init_c_instance(MapField *intern TSRMLS_DC);

void MapField_wrap(MapField *intern, upb_map *map, void *klass);

void MapField___construct(MapField *intern,
                          upb_descriptortype_t key_type,
                          upb_descriptortype_t value_type,
                          void *klass);

PROTO_WRAP_OBJECT_START(MapField)
  upb_map *map;
  void *klass;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// RepeatedField
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(RepeatedField);

void RepeatedField_init(TSRMLS_D);

void RepeatedField_free_c(RepeatedField *intern TSRMLS_DC);
void RepeatedField_init_c_instance(RepeatedField *intern TSRMLS_DC);

void RepeatedField___construct(RepeatedField *intern,
                               upb_descriptortype_t type,
                               void *klass);

void RepeatedField_wrap(RepeatedField *intern, upb_array *arr, void *klass);

PROTO_WRAP_OBJECT_START(RepeatedField)
  upb_array *array;
  void *klass;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// GPBType
// -----------------------------------------------------------------------------

void Type_init(TSRMLS_D);

// -----------------------------------------------------------------------------
// Util
// -----------------------------------------------------------------------------

void Util_init(TSRMLS_D);

// -----------------------------------------------------------------------------
// Help Methods
// -----------------------------------------------------------------------------

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type);

#endif // __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__
