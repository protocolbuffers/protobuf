// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The request handling of the upb conformance testee, shared by the
// conformance_upb binaries and the in-process testee; see the header.

#include "upb/conformance/conformance_upb_harness.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conformance/conformance.upb.h"
#include "conformance/conformance.upbdefs.h"
#include "editions/golden/test_messages_proto2_editions.upbdefs.h"
#include "editions/golden/test_messages_proto3_editions.upbdefs.h"
#include "google/protobuf/test_messages_proto2.upbdefs.h"
#include "google/protobuf/test_messages_proto3.upbdefs.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/internal/def_pool.h"
#include "upb/text/encode.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

/* Set to true to get req/resp printed on stderr. */
static const bool verbose = false;

struct upb_ConformanceHarness {
  upb_DefPool* symtab;
};

typedef struct {
  const conformance_ConformanceRequest* request;
  conformance_ConformanceResponse* response;
  upb_Arena* arena;
  const upb_DefPool* symtab;
} ctx;

static bool parse_proto(upb_Message* msg, const upb_MessageDef* m,
                        const ctx* c) {
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

static void serialize_proto(const upb_Message* msg, const upb_MessageDef* m,
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

static void serialize_text(const upb_Message* msg, const upb_MessageDef* m,
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

static bool parse_json(upb_Message* msg, const upb_MessageDef* m,
                       const ctx* c) {
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

static void serialize_json(const upb_Message* msg, const upb_MessageDef* m,
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
    size_t inerr_len = strlen(inerr);
    char* err = upb_Arena_Malloc(c->arena, inerr_len + 1);
    memcpy(err, inerr, inerr_len);
    err[inerr_len] = '\0';
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

// Reports a request the harness can't act on.  A forked testee used to print
// these to stderr (or exit); as a runtime_error they fail the one test
// instead, in either mode.
static void set_runtime_error(const ctx* c, const char* message) {
  conformance_ConformanceResponse_set_runtime_error(
      c->response, upb_StringView_FromString(message));
}

static bool parse_input(upb_Message* msg, const upb_MessageDef* m,
                        const ctx* c) {
  switch (conformance_ConformanceRequest_payload_case(c->request)) {
    case conformance_ConformanceRequest_payload_protobuf_payload:
      return parse_proto(msg, m, c);
    case conformance_ConformanceRequest_payload_json_payload:
      return parse_json(msg, m, c);
    case conformance_ConformanceRequest_payload_NOT_SET:
      set_runtime_error(c, "conformance_upb: Request didn't have payload.");
      return false;
    default: {
      static const char msg[] = "Unsupported input format.";
      conformance_ConformanceResponse_set_skipped(
          c->response, upb_StringView_FromString(msg));
      return false;
    }
  }
}

static void write_output(const upb_Message* msg, const upb_MessageDef* m,
                         const ctx* c) {
  switch (conformance_ConformanceRequest_requested_output_format(c->request)) {
    case conformance_UNSPECIFIED:
      set_runtime_error(c, "conformance_upb: Unspecified output format.");
      break;
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

static void DoTest(const ctx* c) {
  upb_Message* msg;
  upb_StringView name = conformance_ConformanceRequest_message_type(c->request);
  const upb_MessageDef* m =
      upb_DefPool_FindMessageByNameWithSize(c->symtab, name.data, name.size);

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

static void debug_print(const char* label, const upb_Message* msg,
                        const upb_MessageDef* m, const ctx* c) {
  char buf[512];
  upb_TextEncode(msg, m, c->symtab, UPB_TXTENC_SINGLELINE, buf, sizeof(buf));
  fprintf(stderr, "%s: %s\n", label, buf);
}

upb_ConformanceHarness* upb_ConformanceHarness_New(bool rebuild_minitables) {
  upb_ConformanceHarness* harness = malloc(sizeof(*harness));
  harness->symtab = upb_DefPool_New();

  if (rebuild_minitables) {
    _upb_DefPool_LoadDefInitEx(
        harness->symtab,
        &google_protobuf_test_messages_proto2_proto_upbdefinit, true);
    _upb_DefPool_LoadDefInitEx(
        harness->symtab,
        &google_protobuf_test_messages_proto3_proto_upbdefinit, true);
    _upb_DefPool_LoadDefInitEx(
        harness->symtab,
        &editions_golden_test_messages_proto2_editions_proto_upbdefinit,
        true);
    _upb_DefPool_LoadDefInitEx(
        harness->symtab,
        &editions_golden_test_messages_proto3_editions_proto_upbdefinit,
        true);
  } else {
    protobuf_test_messages_proto2_TestAllTypesProto2_getmsgdef(harness->symtab);
    protobuf_test_messages_editions_proto2_TestAllTypesProto2_getmsgdef(
        harness->symtab);
    protobuf_test_messages_proto3_TestAllTypesProto3_getmsgdef(harness->symtab);
    protobuf_test_messages_editions_proto3_TestAllTypesProto3_getmsgdef(
        harness->symtab);
  }
  return harness;
}

void upb_ConformanceHarness_Free(upb_ConformanceHarness* harness) {
  if (harness == NULL) return;
  upb_DefPool_Free(harness->symtab);
  free(harness);
}

upb_StringView upb_ConformanceHarness_Run(const upb_ConformanceHarness* harness,
                                          upb_StringView serialized_request,
                                          upb_Arena* arena) {
  ctx c;
  char* output;
  size_t output_size;

  c.symtab = harness->symtab;
  c.arena = arena;
  c.request = conformance_ConformanceRequest_parse(
      serialized_request.data, serialized_request.size, arena);
  c.response = conformance_ConformanceResponse_new(arena);

  if (c.request) {
    DoTest(&c);
  } else {
    set_runtime_error(&c,
                      "conformance_upb: parse of ConformanceRequest failed.");
  }

  if (verbose) {
    if (c.request) {
      debug_print("Request", UPB_UPCAST(c.request),
                  conformance_ConformanceRequest_getmsgdef(harness->symtab),
                  &c);
    }
    debug_print("Response", UPB_UPCAST(c.response),
                conformance_ConformanceResponse_getmsgdef(harness->symtab), &c);
    fprintf(stderr, "\n");
  }

  output = conformance_ConformanceResponse_serialize(c.response, arena,
                                                     &output_size);
  return upb_StringView_FromDataAndSize(output, output_size);
}
