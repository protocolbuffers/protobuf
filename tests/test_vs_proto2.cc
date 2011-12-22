/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 *
 * A test that verifies that our results are identical to proto2 for a
 * given proto type and input protobuf.
 */

#define __STDC_LIMIT_MACROS  // So we get UINT32_MAX
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/wire_format_lite.h>
#include "upb/benchmarks/google_messages.pb.h"
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/pb/glue.h"
#include "upb/pb/varint.h"
#include "upb_test.h"

size_t string_size;

void compare(const google::protobuf::Message& proto2_msg,
             void *upb_msg, const upb_msgdef *upb_md);

void compare_arrays(const google::protobuf::Reflection *r,
                    const google::protobuf::Message& proto2_msg,
                    const google::protobuf::FieldDescriptor *proto2_f,
                    void *upb_msg, upb_fielddef *upb_f)
{
  ASSERT(upb_msg_has(upb_msg, upb_f));
  ASSERT(upb_isseq(upb_f));
  const void *arr = upb_value_getptr(upb_msg_getseq(upb_msg, upb_f));
  const void *iter = upb_seq_begin(arr, upb_f);
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
             void *upb_msg, const upb_msgdef *upb_md)
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
                       void *upb_msg, const upb_msgdef *upb_md,
                       const char *str, size_t len, bool allow_jit)
{
  // Parse to both proto2 and upb.
  ASSERT(proto2_msg->ParseFromArray(str, len));
  upb_status status = UPB_STATUS_INIT;
  upb_msg_clear(upb_msg, upb_md);
  upb_strtomsg(str, len, upb_msg, upb_md, allow_jit, &status);
  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing protobuf: %s", upb_status_getstr(&status));
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
  if (argc < 3) {
    fprintf(stderr, "Usage: test_vs_proto2 <descriptor file> <message file>\n");
    return 1;
  }
  const char *descriptor_file = argv[1];
  const char *message_file = argv[2];

  // Initialize upb state, parse descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *symtab = upb_symtab_new();
  size_t fds_len;
  const char *fds = upb_readfile(descriptor_file, &fds_len);
  if(fds == NULL) {
    fprintf(stderr, "Couldn't read %s.\n", descriptor_file);
    return 1;
  }
  upb_load_descriptor_into_symtab(symtab, fds, fds_len, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing %s: %s", descriptor_file,
            upb_status_getstr(&status));
    return 1;
  }
  free((void*)fds);

  const upb_def *def = upb_symtab_lookup(symtab, MESSAGE_NAME);
  const upb_msgdef *msgdef;
  if(!def || !(msgdef = upb_dyncast_msgdef_const(def))) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return 1;
  }

  // Read the message data itself.
  size_t len;
  const char *str = upb_readfile(message_file, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading %s\n", message_file);
    return 1;
  }

  // Run twice to test proper object reuse.
  MESSAGE_CIDENT proto2_msg;
  void *upb_msg = upb_stdmsg_new(msgdef);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len, true);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len, false);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len, true);
  parse_and_compare(&proto2_msg, upb_msg, msgdef, str, len, false);
  printf("All tests passed, %d assertions.\n", num_assertions);

  upb_stdmsg_free(upb_msg, msgdef);
  upb_def_unref(UPB_UPCAST(msgdef));
  free((void*)str);
  upb_symtab_unref(symtab);
  upb_status_uninit(&status);

  // Test Zig-Zag encoding/decoding.
  for (uint64_t num = 5; num * 1.5 > num; num *= 1.5) {
    ASSERT(upb_zzenc_64(num) ==
           google::protobuf::internal::WireFormatLite::ZigZagEncode64(num));
    if (num < UINT32_MAX) {
      ASSERT(upb_zzenc_32(num) ==
             google::protobuf::internal::WireFormatLite::ZigZagEncode32(num));
    }
  }

  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
