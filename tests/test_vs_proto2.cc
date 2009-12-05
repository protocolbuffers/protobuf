
#undef NDEBUG  /* ensure tests always assert. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <google/protobuf/descriptor.h>
#include "upb_parse.h"
#include "upb_def.h"
#include "upb_msg.h"
#include "upb_mm.h"

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  assert(expr); \
  } while(0)

#include MESSAGE_HFILE

void compare(const google::protobuf::Message& proto2_msg,
             struct upb_msg *upb_msg);

void compare_arrays(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    struct upb_msg *upb_msg, struct upb_fielddef *upb_f)
{
  struct upb_array *arr = *upb_msg_getptr(upb_msg, upb_f).arr;
  ASSERT(arr->len == (upb_arraylen_t)r->FieldSize(proto2_msg, proto2_f));
  for(upb_arraylen_t i = 0; i < arr->len; i++) {
    union upb_value_ptr p = upb_array_getelementptr(arr, i);
    switch(upb_f->type) {
      default:
        ASSERT(false);
      case UPB_TYPE(DOUBLE):
        ASSERT(r->GetRepeatedDouble(proto2_msg, proto2_f, i) == *p._double);
        break;
      case UPB_TYPE(FLOAT):
        ASSERT(r->GetRepeatedFloat(proto2_msg, proto2_f, i) == *p._float);
        break;
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64):
        ASSERT(r->GetRepeatedInt64(proto2_msg, proto2_f, i) == *p.int64);
        break;
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64):
        ASSERT(r->GetRepeatedUInt64(proto2_msg, proto2_f, i) == *p.uint64);
        break;
      case UPB_TYPE(SFIXED32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(INT32):
      case UPB_TYPE(ENUM):
        ASSERT(r->GetRepeatedInt32(proto2_msg, proto2_f, i) == *p.int32);
        break;
      case UPB_TYPE(FIXED32):
      case UPB_TYPE(UINT32):
        ASSERT(r->GetRepeatedUInt32(proto2_msg, proto2_f, i) == *p.uint32);
        break;
      case UPB_TYPE(BOOL):
        ASSERT(r->GetRepeatedBool(proto2_msg, proto2_f, i) == *p._bool);
        break;
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES): {
        std::string str = r->GetRepeatedString(proto2_msg, proto2_f, i);
        std::string str2((*p.str)->ptr, (*p.str)->byte_len);
        ASSERT(str == str2);
        break;
      }
      case UPB_TYPE(GROUP):
      case UPB_TYPE(MESSAGE):
        compare(r->GetRepeatedMessage(proto2_msg, proto2_f, i), *p.msg);
    }
  }
}

void compare_values(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    struct upb_msg *upb_msg, struct upb_fielddef *upb_f)
{
  union upb_value_ptr p = upb_msg_getptr(upb_msg, upb_f);
  switch(upb_f->type) {
    default:
      ASSERT(false);
    case UPB_TYPE(DOUBLE):
      ASSERT(r->GetDouble(proto2_msg, proto2_f) == *p._double);
      break;
    case UPB_TYPE(FLOAT):
      ASSERT(r->GetFloat(proto2_msg, proto2_f) == *p._float);
      break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SINT64):
    case UPB_TYPE(SFIXED64):
      ASSERT(r->GetInt64(proto2_msg, proto2_f) == *p.int64);
      break;
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      ASSERT(r->GetUInt64(proto2_msg, proto2_f) == *p.uint64);
      break;
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(INT32):
    case UPB_TYPE(ENUM):
      ASSERT(r->GetInt32(proto2_msg, proto2_f) == *p.int32);
      break;
    case UPB_TYPE(FIXED32):
    case UPB_TYPE(UINT32):
      ASSERT(r->GetUInt32(proto2_msg, proto2_f) == *p.uint32);
      break;
    case UPB_TYPE(BOOL):
      ASSERT(r->GetBool(proto2_msg, proto2_f) == *p._bool);
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES): {
      std::string str = r->GetString(proto2_msg, proto2_f);
      std::string str2((*p.str)->ptr, (*p.str)->byte_len);
      ASSERT(str == str2);
      break;
    }
    case UPB_TYPE(GROUP):
    case UPB_TYPE(MESSAGE):
      compare(r->GetMessage(proto2_msg, proto2_f), *p.msg);
  }
}

void compare(const google::protobuf::Message& proto2_msg,
             struct upb_msg *upb_msg)
{
  const google::protobuf::Reflection *r = proto2_msg.GetReflection();
  const google::protobuf::Descriptor *d = proto2_msg.GetDescriptor();
  struct upb_msgdef *def = upb_msg->def;

  ASSERT((uint32_t)d->field_count() == def->num_fields);
  for(uint32_t i = 0; i < def->num_fields; i++) {
    struct upb_fielddef *upb_f = &def->fields[i];
    const google::protobuf::FieldDescriptor *proto2_f =
        d->FindFieldByNumber(upb_f->number);
    // Make sure the definitions are equal.
    ASSERT(upb_f);
    ASSERT(proto2_f);
    ASSERT(upb_f->number == proto2_f->number());
    ASSERT(std::string(upb_f->name->ptr, upb_f->name->byte_len) ==
           proto2_f->name());
    ASSERT(upb_f->type == proto2_f->type());
    ASSERT(upb_isarray(upb_f) == proto2_f->is_repeated());

    if(!upb_msg_isset(upb_msg, upb_f)) {
      if(upb_isarray(upb_f))
        ASSERT(r->FieldSize(proto2_msg, proto2_f) == 0);
      else
        ASSERT(r->HasField(proto2_msg, proto2_f) == false);
    } else {
      if(upb_isarray(upb_f))
        compare_arrays(r, proto2_msg, proto2_f, upb_msg, upb_f);
      else
        compare_values(r, proto2_msg, proto2_f, upb_msg, upb_f);
    }
  }
}

void parse_and_compare(MESSAGE_CIDENT *proto2_msg, struct upb_msg *upb_msg,
                       struct upb_string *str)
{
  // Parse to both proto2 and upb.
  ASSERT(proto2_msg->ParseFromArray(str->ptr, str->byte_len));
  struct upb_status status = UPB_STATUS_INIT;
  upb_msg_parsestr(upb_msg, str->ptr, str->byte_len, &status);
  ASSERT(upb_ok(&status));
  compare(*proto2_msg, upb_msg);
}

int main(int argc, char *argv[])
{
  // Change cwd to where the binary is.
  (void)argc;
  char *lastslash = strrchr(argv[0], '/');
  char *progname = argv[0];
  if(lastslash) {
    *lastslash = '\0';
    if(chdir(argv[0]) < 0) {
      fprintf(stderr, "Error changing directory to %s.\n", argv[0]);
      return 1;
    }
    *lastslash = '/';
    progname = lastslash + 3;  /* "/b_" */
  }

  // Initialize upb state, parse descriptor.
  struct upb_status status = UPB_STATUS_INIT;
  struct upb_symtab *c = upb_symtab_new();
  struct upb_string *fds = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(!fds) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ".\n");
    return 1;
  }
  upb_symtab_add_desc(c, fds, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ": %s.\n",
            status.msg);
    return 1;
  }
  upb_string_unref(fds);

  struct upb_string *proto_name = upb_strdupc(MESSAGE_NAME);
  struct upb_msgdef *def = upb_downcast_msgdef(upb_symtab_lookup(c, proto_name));
  if(!def) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(proto_name));
    return 1;
  }
  upb_string_unref(proto_name);

  // Read the message data itself.
  struct upb_string *str = upb_strreadfile(MESSAGE_FILE);
  if(!str) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return 1;
  }

  // Run twice to test proper object reuse.
  MESSAGE_CIDENT proto2_msg;
  struct upb_msg *upb_msg = upb_msg_new(def);
  parse_and_compare(&proto2_msg, upb_msg, str);
  parse_and_compare(&proto2_msg, upb_msg, str);
  printf("All tests passed, %d assertions.\n", num_assertions);

  upb_msg_unref(upb_msg);
  upb_string_unref(str);
  upb_symtab_unref(c);

  return 0;
}
