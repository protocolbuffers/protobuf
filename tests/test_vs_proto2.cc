
#undef NDEBUG  /* ensure tests always assert. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <google/protobuf/descriptor.h>
#include "upb_decoder.h"
#include "upb_def.h"
#include "upb_glue.h"
#include "upb_msg.h"
#include "upb_strstream.h"

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  assert(expr); \
  } while(0)

#include MESSAGE_HFILE

void compare(const google::protobuf::Message& proto2_msg,
             upb_msg *upb_msg, upb_msgdef *upb_md);

void compare_arrays(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    upb_msg *upb_msg, upb_fielddef *upb_f)
{
  ASSERT(upb_msg_has(upb_msg, upb_f));
  upb_array *arr = upb_value_getarr(upb_msg_get(upb_msg, upb_f));
  ASSERT(upb_array_len(arr) == (upb_arraylen_t)r->FieldSize(proto2_msg, proto2_f));
  for(upb_arraylen_t i = 0; i < upb_array_len(arr); i++) {
    upb_value v = upb_array_get(arr, upb_f, i);
    switch(upb_f->type) {
      default:
        ASSERT(false);
      case UPB_TYPE(DOUBLE):
        ASSERT(r->GetRepeatedDouble(proto2_msg, proto2_f, i) == upb_value_getdouble(v));
        break;
      case UPB_TYPE(FLOAT):
        ASSERT(r->GetRepeatedFloat(proto2_msg, proto2_f, i) == upb_value_getfloat(v));
        break;
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64):
        ASSERT(r->GetRepeatedInt64(proto2_msg, proto2_f, i) == upb_value_getint64(v));
        break;
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64):
        ASSERT(r->GetRepeatedUInt64(proto2_msg, proto2_f, i) == upb_value_getuint64(v));
        break;
      case UPB_TYPE(SFIXED32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(INT32):
      case UPB_TYPE(ENUM):
        ASSERT(r->GetRepeatedInt32(proto2_msg, proto2_f, i) == upb_value_getint32(v));
        break;
      case UPB_TYPE(FIXED32):
      case UPB_TYPE(UINT32):
        ASSERT(r->GetRepeatedUInt32(proto2_msg, proto2_f, i) == upb_value_getuint32(v));
        break;
      case UPB_TYPE(BOOL):
        ASSERT(r->GetRepeatedBool(proto2_msg, proto2_f, i) == upb_value_getbool(v));
        break;
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES): {
        std::string str = r->GetRepeatedString(proto2_msg, proto2_f, i);
        upb_string *upbstr = upb_value_getstr(v);
        std::string str2(upb_string_getrobuf(upbstr), upb_string_len(upbstr));
        ASSERT(str == str2);
        break;
      }
      case UPB_TYPE(GROUP):
      case UPB_TYPE(MESSAGE):
        ASSERT(upb_dyncast_msgdef(upb_f->def) != NULL);
        compare(r->GetRepeatedMessage(proto2_msg, proto2_f, i),
                upb_value_getmsg(v), upb_downcast_msgdef(upb_f->def));
    }
  }
}

#include <inttypes.h>
void compare_values(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    upb_msg *upb_msg, upb_fielddef *upb_f)
{
  upb_value v = upb_msg_get(upb_msg, upb_f);
  switch(upb_f->type) {
    default:
      ASSERT(false);
    case UPB_TYPE(DOUBLE):
      ASSERT(r->GetDouble(proto2_msg, proto2_f) == upb_value_getdouble(v));
      break;
    case UPB_TYPE(FLOAT):
      ASSERT(r->GetFloat(proto2_msg, proto2_f) == upb_value_getfloat(v));
      break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SINT64):
    case UPB_TYPE(SFIXED64):
      ASSERT(r->GetInt64(proto2_msg, proto2_f) == upb_value_getint64(v));
      break;
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      ASSERT(r->GetUInt64(proto2_msg, proto2_f) == upb_value_getuint64(v));
      break;
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(INT32):
    case UPB_TYPE(ENUM):
      ASSERT(r->GetInt32(proto2_msg, proto2_f) == upb_value_getint32(v));
      break;
    case UPB_TYPE(FIXED32):
    case UPB_TYPE(UINT32):
      ASSERT(r->GetUInt32(proto2_msg, proto2_f) == upb_value_getuint32(v));
      break;
    case UPB_TYPE(BOOL):
      ASSERT(r->GetBool(proto2_msg, proto2_f) == upb_value_getbool(v));
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES): {
      std::string str = r->GetString(proto2_msg, proto2_f);
      upb_string *upbstr = upb_value_getstr(v);
      std::string str2(upb_string_getrobuf(upbstr), upb_string_len(upbstr));
      ASSERT(str == str2);
      break;
    }
    case UPB_TYPE(GROUP):
    case UPB_TYPE(MESSAGE):
      compare(r->GetMessage(proto2_msg, proto2_f),
              upb_value_getmsg(v), upb_downcast_msgdef(upb_f->def));
  }
}

void compare(const google::protobuf::Message& proto2_msg,
             upb_msg *upb_msg, upb_msgdef *upb_md)
{
  const google::protobuf::Reflection *r = proto2_msg.GetReflection();
  const google::protobuf::Descriptor *d = proto2_msg.GetDescriptor();

  ASSERT((upb_field_count_t)d->field_count() == upb_msgdef_numfields(upb_md));
  upb_msg_iter i;
  for(i = upb_msg_begin(upb_md); !upb_msg_done(i); i = upb_msg_next(upb_md, i)) {
    upb_fielddef *upb_f = upb_msg_iter_field(i);
    const google::protobuf::FieldDescriptor *proto2_f =
        d->FindFieldByNumber(upb_f->number);
    // Make sure the definitions are equal.
    ASSERT(upb_f);
    ASSERT(proto2_f);
    ASSERT(upb_f->number == proto2_f->number());
    ASSERT(std::string(upb_string_getrobuf(upb_f->name),
                       upb_string_len(upb_f->name)) ==
           proto2_f->name());
    ASSERT(upb_f->type == proto2_f->type());
    ASSERT(upb_isarray(upb_f) == proto2_f->is_repeated());

    if(!upb_msg_has(upb_msg, upb_f)) {
      if(upb_isarray(upb_f))
        ASSERT(r->FieldSize(proto2_msg, proto2_f) == 0);
      else
        ASSERT(r->HasField(proto2_msg, proto2_f) == false);
    } else {
      if(upb_isarray(upb_f)) {
        compare_arrays(r, proto2_msg, proto2_f, upb_msg, upb_f);
      } else {
        ASSERT(r->HasField(proto2_msg, proto2_f) == true);
        compare_values(r, proto2_msg, proto2_f, upb_msg, upb_f);
      }
    }
  }
}

void parse_and_compare(MESSAGE_CIDENT *proto2_msg,
                       upb_msg *upb_msg, upb_msgdef *upb_md,
                       upb_string *str)
{
  // Parse to both proto2 and upb.
  ASSERT(proto2_msg->ParseFromArray(upb_string_getrobuf(str), upb_string_len(str)));
  upb_status status = UPB_STATUS_INIT;
  upb_msg_clear(upb_msg, upb_md);
  upb_strtomsg(str, upb_msg, upb_md, &status);
  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing test protobuf: ");
    upb_printerr(&status);
    exit(1);
  }
  compare(*proto2_msg, upb_msg, upb_md);
  upb_status_uninit(&status);
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
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *symtab = upb_symtab_new();
  upb_string *fds = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(fds == NULL) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ".\n");
    return 1;
  }
  upb_symtab_add_descriptorproto(symtab);
  upb_def *fds_msgdef = upb_symtab_lookup(
      symtab, UPB_STRLIT("google.protobuf.FileDescriptorSet"));
  assert(fds_msgdef);

  upb_stringsrc ssrc;
  upb_stringsrc_init(&ssrc);
  upb_stringsrc_reset(&ssrc, fds);
  upb_decoder decoder;
  upb_decoder_init(&decoder, upb_downcast_msgdef(fds_msgdef));
  upb_decoder_reset(&decoder, upb_stringsrc_bytesrc(&ssrc));
  upb_symtab_addfds(symtab, upb_decoder_src(&decoder), &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ": ");
    upb_printerr(&status);
    return 1;
  }
  upb_string_unref(fds);
  upb_decoder_uninit(&decoder);
  upb_stringsrc_uninit(&ssrc);

  upb_string *proto_name = upb_strdupc(MESSAGE_NAME);
  upb_def *def = upb_symtab_lookup(symtab, proto_name);
  upb_msgdef *msgdef;
  if(!def || !(msgdef = upb_dyncast_msgdef(def))) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(proto_name));
    return 1;
  }
  upb_string_unref(proto_name);

  // Read the message data itself.
  upb_string *str = upb_strreadfile(MESSAGE_FILE);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return 1;
  }

  // Run twice to test proper object reuse.
  MESSAGE_CIDENT proto2_msg;
  upb_msg *upb_msg = upb_msg_new(msgdef);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str);
  printf("All tests passed, %d assertions.\n", num_assertions);

  upb_msg_unref(upb_msg, msgdef);
  upb_def_unref(UPB_UPCAST(msgdef));
  upb_def_unref(fds_msgdef);
  upb_string_unref(str);
  upb_symtab_unref(symtab);
  upb_status_uninit(&status);
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
