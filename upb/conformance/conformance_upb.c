// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* This is a upb implementation of the upb conformance tests, see:
 *   https://github.com/google/protobuf/tree/master/conformance
 */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "conformance/conformance.upb.h"
#include "conformance/conformance.upbdefs.h"
#include "google/protobuf/editions/golden/test_messages_proto2_editions.upbdefs.h"
#include "google/protobuf/editions/golden/test_messages_proto3_editions.upbdefs.h"
#include "google/protobuf/test_messages_proto2.upbdefs.h"
#include "google/protobuf/test_messages_proto3.upbdefs.h"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/reflection/message.h"
#include "upb/text/encode.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

int test_count = 0;
bool verbose = false; /* Set to true to get req/resp printed on stderr. */

bool CheckedRead(int fd, void* buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    ssize_t bytes_read = read(fd, (char*)buf + ofs, len);

    if (bytes_read == 0) return false;

    if (bytes_read < 0) {
      perror("reading from test runner");
      exit(1);
    }

    len -= bytes_read;
    ofs += bytes_read;
  }

  return true;
}

void CheckedWrite(int fd, const void* buf, size_t len) {
  if ((size_t)write(fd, buf, len) != len) {
    perror("writing to test runner");
    exit(1);
  }
}

typedef struct {
  const conformance_ConformanceRequest* request;
  conformance_ConformanceResponse* response;
  upb_Arena* arena;
  const upb_DefPool* symtab;
} ctx;

bool parse_proto(upb_Message* msg, const upb_MessageDef* m, const ctx* c) {
  upb_StringView proto =
      conformance_ConformanceRequest_protobuf_payload(c->request);
  if (upb_Decode(proto.data, proto.size, msg, upb_MessageDef_MiniTable(m), NULL,
                 0, c->arena) == kUpb_DecodeStatus_Ok) {
    return true;
  } else {
    static const char msg[] = "Parse error";
    conformance_ConformanceResponse_set_parse_error(
        c->response, upb_StringView_FromString(msg));
    return false;
  }
}

void serialize_proto(const upb_Message* msg, const upb_MessageDef* m,
                     const ctx* c) {
  size_t len;
  char* data;
  upb_EncodeStatus status =
      upb_Encode(msg, upb_MessageDef_MiniTable(m), 0, c->arena, &data, &len);
  if (status == kUpb_EncodeStatus_Ok) {
    conformance_ConformanceResponse_set_protobuf_payload(
        c->response, upb_StringView_FromDataAndSize(data, len));
  } else {
    static const char msg[] = "Error serializing.";
    conformance_ConformanceResponse_set_serialize_error(
        c->response, upb_StringView_FromString(msg));
  }
}

void serialize_text(const upb_Message* msg, const upb_MessageDef* m,
                    const ctx* c) {
  size_t len;
  size_t len2;
  int opts = 0;
  char* data;

  if (!conformance_ConformanceRequest_print_unknown_fields(c->request)) {
    opts |= UPB_TXTENC_SKIPUNKNOWN;
  }

  len = upb_TextEncode(msg, m, c->symtab, opts, NULL, 0);
  data = upb_Arena_Malloc(c->arena, len + 1);
  len2 = upb_TextEncode(msg, m, c->symtab, opts, data, len + 1);
  UPB_ASSERT(len == len2);
  conformance_ConformanceResponse_set_text_payload(
      c->response, upb_StringView_FromDataAndSize(data, len));
}

bool parse_json(upb_Message* msg, const upb_MessageDef* m, const ctx* c) {
  upb_StringView json = conformance_ConformanceRequest_json_payload(c->request);
  upb_Status status;
  int opts = 0;

  if (conformance_ConformanceRequest_test_category(c->request) ==
      conformance_JSON_IGNORE_UNKNOWN_PARSING_TEST) {
    opts |= upb_JsonDecode_IgnoreUnknown;
  }

  upb_Status_Clear(&status);
  if (upb_JsonDecode(json.data, json.size, msg, m, c->symtab, opts, c->arena,
                     &status)) {
    return true;
  } else {
    const char* inerr = upb_Status_ErrorMessage(&status);
    size_t len = strlen(inerr);
    char* err = upb_Arena_Malloc(c->arena, len + 1);
    memcpy(err, inerr, strlen(inerr));
    err[len] = '\0';
    conformance_ConformanceResponse_set_parse_error(
        c->response, upb_StringView_FromString(err));
    return false;
  }
}

void serialize_json(const upb_Message* msg, const upb_MessageDef* m,
                    const ctx* c) {
  size_t len;
  size_t len2;
  int opts = 0;
  char* data;
  upb_Status status;

  upb_Status_Clear(&status);
  len = upb_JsonEncode(msg, m, c->symtab, opts, NULL, 0, &status);

  if (len == (size_t)-1) {
    const char* inerr = upb_Status_ErrorMessage(&status);
    size_t len = strlen(inerr);
    char* err = upb_Arena_Malloc(c->arena, len + 1);
    memcpy(err, inerr, strlen(inerr));
    err[len] = '\0';
    conformance_ConformanceResponse_set_serialize_error(
        c->response, upb_StringView_FromString(err));
    return;
  }

  data = upb_Arena_Malloc(c->arena, len + 1);
  len2 = upb_JsonEncode(msg, m, c->symtab, opts, data, len + 1, &status);
  UPB_ASSERT(len == len2);
  conformance_ConformanceResponse_set_json_payload(
      c->response, upb_StringView_FromDataAndSize(data, len));
}

bool parse_input(upb_Message* msg, const upb_MessageDef* m, const ctx* c) {
  switch (conformance_ConformanceRequest_payload_case(c->request)) {
    case conformance_ConformanceRequest_payload_protobuf_payload:
      return parse_proto(msg, m, c);
    case conformance_ConformanceRequest_payload_json_payload:
      return parse_json(msg, m, c);
    case conformance_ConformanceRequest_payload_NOT_SET:
      fprintf(stderr, "conformance_upb: Request didn't have payload.\n");
      return false;
    default: {
      static const char msg[] = "Unsupported input format.";
      conformance_ConformanceResponse_set_skipped(
          c->response, upb_StringView_FromString(msg));
      return false;
    }
  }
}

void write_output(const upb_Message* msg, const upb_MessageDef* m,
                  const ctx* c) {
  switch (conformance_ConformanceRequest_requested_output_format(c->request)) {
    case conformance_UNSPECIFIED:
      fprintf(stderr, "conformance_upb: Unspecified output format.\n");
      exit(1);
    case conformance_PROTOBUF:
      serialize_proto(msg, m, c);
      break;
    case conformance_TEXT_FORMAT:
      serialize_text(msg, m, c);
      break;
    case conformance_JSON:
      serialize_json(msg, m, c);
      break;
    default: {
      static const char msg[] = "Unsupported output format.";
      conformance_ConformanceResponse_set_skipped(
          c->response, upb_StringView_FromString(msg));
      break;
    }
  }
}

void DoTest(const ctx* c) {
  upb_Message* msg;
  upb_StringView name = conformance_ConformanceRequest_message_type(c->request);
  const upb_MessageDef* m =
      upb_DefPool_FindMessageByNameWithSize(c->symtab, name.data, name.size);
#if 0
  // Handy code for limiting conformance tests to a single input payload.
  // This is a hack since the conformance runner doesn't give an easy way to
  // specify what test should be run.
  const char skip[] = "\343>\010\301\002\344>\230?\001\230?\002\230?\003";
  upb_StringView skip_str = upb_StringView_FromDataAndSize(skip, sizeof(skip) - 1);
  upb_StringView pb_payload =
      conformance_ConformanceRequest_protobuf_payload(c->request);
  if (!upb_StringView_IsEqual(pb_payload, skip_str)) m = NULL;
#endif

  if (!m) {
    static const char msg[] = "Unknown message type.";
    conformance_ConformanceResponse_set_skipped(c->response,
                                                upb_StringView_FromString(msg));
    return;
  }

  msg = upb_Message_New(upb_MessageDef_MiniTable(m), c->arena);

  if (parse_input(msg, m, c)) {
    write_output(msg, m, c);
  }
}

void debug_print(const char* label, const upb_Message* msg,
                 const upb_MessageDef* m, const ctx* c) {
  char buf[512];
  upb_TextEncode(msg, m, c->symtab, UPB_TXTENC_SINGLELINE, buf, sizeof(buf));
  fprintf(stderr, "%s: %s\n", label, buf);
}

bool DoTestIo(upb_DefPool* symtab) {
  upb_Status status;
  char* input;
  char* output;
  uint32_t input_size;
  size_t output_size;
  ctx c;

  if (!CheckedRead(STDIN_FILENO, &input_size, sizeof(uint32_t))) {
    /* EOF. */
    return false;
  }

  c.symtab = symtab;
  c.arena = upb_Arena_New();
  input = upb_Arena_Malloc(c.arena, input_size);

  if (!CheckedRead(STDIN_FILENO, input, input_size)) {
    fprintf(stderr, "conformance_upb: unexpected EOF on stdin.\n");
    exit(1);
  }

  c.request = conformance_ConformanceRequest_parse(input, input_size, c.arena);
  c.response = conformance_ConformanceResponse_new(c.arena);

  if (c.request) {
    DoTest(&c);
  } else {
    fprintf(stderr, "conformance_upb: parse of ConformanceRequest failed: %s\n",
            upb_Status_ErrorMessage(&status));
  }

  output = conformance_ConformanceResponse_serialize(c.response, c.arena,
                                                     &output_size);

  uint32_t network_out = (uint32_t)output_size;
  CheckedWrite(STDOUT_FILENO, &network_out, sizeof(uint32_t));
  CheckedWrite(STDOUT_FILENO, output, output_size);

  test_count++;

  if (verbose) {
    debug_print("Request", UPB_UPCAST(c.request),
                conformance_ConformanceRequest_getmsgdef(symtab), &c);
    debug_print("Response", UPB_UPCAST(c.response),
                conformance_ConformanceResponse_getmsgdef(symtab), &c);
    fprintf(stderr, "\n");
  }

  upb_Arena_Free(c.arena);

  return true;
}

int main(void) {
  upb_DefPool* symtab = upb_DefPool_New();

#ifdef REBUILD_MINITABLES
  _upb_DefPool_LoadDefInitEx(
      symtab, &google_protobuf_test_messages_proto2_proto_upbdefinit, true);
  _upb_DefPool_LoadDefInitEx(
      symtab, &google_protobuf_test_messages_proto3_proto_upbdefinit, true);
  _upb_DefPool_LoadDefInitEx(
      symtab,
      &google_protobuf_editions_golden_test_messages_proto2_editions_proto_upbdefinit,
      true);
  _upb_DefPool_LoadDefInitEx(
      symtab,
      &google_protobuf_editions_golden_test_messages_proto3_editions_proto_upbdefinit,
      true);
#else
  protobuf_test_messages_proto2_TestAllTypesProto2_getmsgdef(symtab);
  protobuf_test_messages_editions_proto2_TestAllTypesProto2_getmsgdef(symtab);
  protobuf_test_messages_proto3_TestAllTypesProto3_getmsgdef(symtab);
  protobuf_test_messages_editions_proto3_TestAllTypesProto3_getmsgdef(symtab);
#endif

  while (1) {
    if (!DoTestIo(symtab)) {
      fprintf(stderr,
              "conformance_upb: received EOF from test runner "
              "after %d tests, exiting\n",
              test_count);
      upb_DefPool_Free(symtab);
      return 0;
    }
  }
}
