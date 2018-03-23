#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__

#include <string>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>

#include "port.h"
#include "upb.h"

#define PHP_PROTOBUF_EXTNAME "protobuf"
#define PHP_PROTOBUF_VERSION "3.4.1"

// -----------------------------------------------------------------------------
// Global Utils
// -----------------------------------------------------------------------------

void register_upbdef(const char* classname, const upb_def* def);
const upb_msgdef* class2msgdef(const void* klass);
const void* msgdef2class(const upb_msgdef* msgdef);

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
// Arena
// -----------------------------------------------------------------------------

struct proto_arena {
  upb_arena arena;
  PHP_OBJECT wrapper;
};

typedef proto_arena proto_arena;

void proto_arena_init(proto_arena *arena);
void proto_arena_uninit(proto_arena *arena);

PROTO_FORWARD_DECLARE_CLASS(Arena);

void Arena_init(TSRMLS_D);

void Arena_free_c(Arena *object TSRMLS_DC);
void Arena_init_c_instance(Arena *object TSRMLS_DC);

PROTO_WRAP_OBJECT_START(Arena)
  upb_arena *arena;
PROTO_WRAP_OBJECT_END

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
void Message_deepclean(upb_msg *msg, const upb_msgdef *m);

void Message_wrap(Message *intern, upb_msg *msg, const upb_msgdef *msgde);

void Message___construct(Message *intern, const upb_msgdef *msgdef,
                         upb_arena *arena_parent);
void Message_mergeFromString(
    Message *intern, const char *data, size_t size);

// TODO(teboring): Combine all metadata.
PROTO_WRAP_OBJECT_START(Message)
  const upb_msgdef *msgdef;
  const upb_msglayout *layout;
  upb_msg *msg;
  ARENA arena;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(MapField);

void MapField_init(TSRMLS_D);

void MapField_free_c(MapField *intern TSRMLS_DC);
void MapField_init_c_instance(MapField *intern TSRMLS_DC);
void MapField_deepclean(upb_map *map, const upb_msgdef *m);

void MapField_wrap(MapField *intern, upb_map *map, void *klass);

void MapField___construct(MapField *intern,
                          upb_descriptortype_t key_type,
                          upb_descriptortype_t value_type,
                          ARENA arena,
                          void *klass);

PROTO_WRAP_OBJECT_START(MapField)
  upb_map *map;
  void *klass;
  ARENA arena;
  std::unordered_map<void*, PHP_OBJECT>* wrappers;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(MapFieldIter);

void MapFieldIter_init(TSRMLS_D);

void MapFieldIter_free_c(MapFieldIter *intern TSRMLS_DC);
void MapFieldIter_init_c_instance(MapFieldIter *intern TSRMLS_DC);

PROTO_WRAP_OBJECT_START(MapFieldIter)
  MapField* map_field;
  upb_mapiter* iter;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// RepeatedField
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(RepeatedField);

void RepeatedField_init(TSRMLS_D);

void RepeatedField_free_c(RepeatedField *intern TSRMLS_DC);
void RepeatedField_init_c_instance(RepeatedField *intern TSRMLS_DC);
void RepeatedField_deepclean(upb_array *array, const upb_msgdef *m);

void RepeatedField___construct(RepeatedField *intern,
                               upb_descriptortype_t type,
                               ARENA arena,
                               void *klass);

void RepeatedField_wrap(RepeatedField *intern, upb_array *arr, void *klass);

PROTO_WRAP_OBJECT_START(RepeatedField)
  upb_array *array;
  void *klass;
  ARENA arena;
  std::unordered_map<void*, PHP_OBJECT>* wrappers;
PROTO_WRAP_OBJECT_END

// -----------------------------------------------------------------------------
// RepeatedFieldIter
// -----------------------------------------------------------------------------

PROTO_FORWARD_DECLARE_CLASS(RepeatedFieldIter);

void RepeatedFieldIter_init(TSRMLS_D);

void RepeatedFieldIter_free_c(RepeatedFieldIter *intern TSRMLS_DC);
void RepeatedFieldIter_init_c_instance(RepeatedFieldIter *intern TSRMLS_DC);

PROTO_WRAP_OBJECT_START(RepeatedFieldIter)
  RepeatedField* repeated_field;
  int position;
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

// Stack-allocated context during an encode/decode operation. Contains the upb
// environment and its stack-based allocator, an initial buffer for allocations
// to avoid malloc() when possible, and a template for PHP exception messages
// if any error occurs.
#define STACK_ENV_STACKBYTES 4096
typedef struct {
  upb_env env;
  const char *php_error_template;
  char allocbuf[STACK_ENV_STACKBYTES];
} stackenv;

void stackenv_init(stackenv* se, const char* errmsg);
void stackenv_uninit(stackenv* se);

#endif // __GOOGLE_PROTOBUF_PHP_PROTOBUF_CPP_H__
