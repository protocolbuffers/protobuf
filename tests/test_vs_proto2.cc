/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 *
 * A test that verifies that our results are identical to proto2 for a
 * given proto type and input protobuf.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <google/protobuf/descriptor.h>
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/pb/glue.h"
#include "upb_test.h"

#include MESSAGE_HFILE

size_t string_size;

void compare(const google::protobuf::Message& proto2_msg,
             void *upb_msg, upb_msgdef *upb_md);

void compare_arrays(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    void *upb_msg, upb_fielddef *upb_f)
{
  ASSERT(upb_msg_has(upb_msg, upb_f));
  ASSERT(upb_isseq(upb_f));
  void *arr = upb_value_getptr(upb_msg_getseq(upb_msg, upb_f));
  void *iter = upb_seq_begin(arr, upb_f);
  for(int i = 0;
      i < r->FieldSize(proto2_msg, proto2_f);
      i++, iter = upb_seq_next(arr, iter, upb_f)) {
    ASSERT(!upb_seq_done(iter));
    upb_value v = upb_seq_get(iter, upb_f);
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
        upb_stdarray *upbstr = (upb_stdarray*)upb_value_getptr(v);
        std::string str2(upbstr->ptr, upbstr->len);
        string_size += upbstr->len;
        ASSERT(str == str2);
        break;
      }
      case UPB_TYPE(GROUP):
      case UPB_TYPE(MESSAGE):
        ASSERT(upb_dyncast_msgdef(upb_f->def) != NULL);
        compare(r->GetRepeatedMessage(proto2_msg, proto2_f, i),
                upb_value_getptr(v), upb_downcast_msgdef(upb_f->def));
    }
  }
  ASSERT(upb_seq_done(iter));
}

#include <inttypes.h>
void compare_values(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    void *upb_msg, upb_fielddef *upb_f)
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
      upb_stdarray *upbstr = (upb_stdarray*)upb_value_getptr(v);
      std::string str2(upbstr->ptr, upbstr->len);
      string_size += upbstr->len;
      ASSERT(str == str2);
      break;
    }
    case UPB_TYPE(GROUP):
    case UPB_TYPE(MESSAGE):
      // XXX: getstr
      compare(r->GetMessage(proto2_msg, proto2_f),
              upb_value_getptr(v), upb_downcast_msgdef(upb_f->def));
  }
}

void compare(const google::protobuf::Message& proto2_msg,
             void *upb_msg, upb_msgdef *upb_md)
{
  const google::protobuf::Reflection *r = proto2_msg.GetReflection();
  const google::protobuf::Descriptor *d = proto2_msg.GetDescriptor();

  ASSERT(d->field_count() == upb_msgdef_numfields(upb_md));
  upb_msg_iter i;
  for(i = upb_msg_begin(upb_md); !upb_msg_done(i); i = upb_msg_next(upb_md, i)) {
    upb_fielddef *upb_f = upb_msg_iter_field(i);
    const google::protobuf::FieldDescriptor *proto2_f =
        d->FindFieldByNumber(upb_f->number);
    // Make sure the definitions are equal.
    ASSERT(upb_f);
    ASSERT(proto2_f);
    ASSERT(upb_f->number == proto2_f->number());
    ASSERT(std::string(upb_f->name) == proto2_f->name());
    ASSERT(upb_f->type == proto2_f->type());
    ASSERT(upb_isseq(upb_f) == proto2_f->is_repeated());

    if(!upb_msg_has(upb_msg, upb_f)) {
      if(upb_isseq(upb_f))
        ASSERT(r->FieldSize(proto2_msg, proto2_f) == 0);
      else
        ASSERT(r->HasField(proto2_msg, proto2_f) == false);
    } else {
      if(upb_isseq(upb_f)) {
        compare_arrays(r, proto2_msg, proto2_f, upb_msg, upb_f);
      } else {
        ASSERT(r->HasField(proto2_msg, proto2_f) == true);
        compare_values(r, proto2_msg, proto2_f, upb_msg, upb_f);
      }
    }
  }
}

void parse_and_compare(MESSAGE_CIDENT *proto2_msg,
                       void *upb_msg, upb_msgdef *upb_md,
                       const char *str, size_t len)
{
  // Parse to both proto2 and upb.
  ASSERT(proto2_msg->ParseFromArray(str, len));
  upb_status status = UPB_STATUS_INIT;
  upb_msg_clear(upb_msg, upb_md);
  upb_strtomsg(str, len, upb_msg, upb_md, &status);
  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing test protobuf: ");
    upb_status_print(&status, stderr);
    exit(1);
  }
  string_size = 0;
  compare(*proto2_msg, upb_msg, upb_md);
  printf("Total size: %zd, string size: %zd (%0.2f%%)\n", len,
         string_size, (double)string_size / len * 100);
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
  size_t fds_len;
  const char *fds = upb_readfile(MESSAGE_DESCRIPTOR_FILE, &fds_len);
  if(fds == NULL) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ".\n");
    return 1;
  }
  upb_read_descriptor(symtab, fds, fds_len, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ": ");
    upb_status_print(&status, stderr);
    return 1;
  }
  free((void*)fds);

  upb_def *def = upb_symtab_lookup(symtab, MESSAGE_NAME);
  upb_msgdef *msgdef;
  if(!def || !(msgdef = upb_dyncast_msgdef(def))) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return 1;
  }

  // Read the message data itself.
  size_t len;
  const char *str = upb_readfile(MESSAGE_FILE, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return 1;
  }

  // Run twice to test proper object reuse.
  MESSAGE_CIDENT proto2_msg;
  void *upb_msg = upb_stdmsg_new(msgdef);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len);
  printf("All tests passed, %d assertions.\n", num_assertions);

  upb_stdmsg_free(upb_msg, msgdef);
  upb_def_unref(UPB_UPCAST(msgdef));
  free((void*)str);
  upb_symtab_unref(symtab);
  upb_status_uninit(&status);
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
