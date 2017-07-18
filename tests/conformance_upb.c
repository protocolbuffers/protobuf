/* This is a upb implementation of the upb conformance tests, see:
 *   https://github.com/google/protobuf/tree/master/conformance
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "conformance.upb.h"
#include "google/protobuf/test_messages_proto3.upb.h"

int test_count = 0;

bool CheckedRead(int fd, void *buf, size_t len) {
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

void CheckedWrite(int fd, const void *buf, size_t len) {
  if (write(fd, buf, len) != len) {
    perror("writing to test runner");
    exit(1);
  }
}

void DoTest(
    const conformance_ConformanceRequest* request,
    conformance_ConformanceResponse *response,
    upb_env *env) {
      conformance_ConformanceResponse_new(env);
  protobuf_test_messages_proto3_TestAllTypes *test_message;

  switch (conformance_ConformanceRequest_payload_case(request)) {
    case conformance_ConformanceRequest_payload_protobuf_payload:
      test_message = protobuf_test_messages_proto3_TestAllTypes_parsenew(
          conformance_ConformanceRequest_protobuf_payload(request), env);

      if (!test_message) {
        /* TODO(haberman): return details. */
        static const char msg[] = "Parse error (no more details available).";
        conformance_ConformanceResponse_set_parse_error(
            response, upb_stringview_make(msg, sizeof(msg)));
        return;
      }
      break;

    case conformance_ConformanceRequest_payload_json_payload: {
      static const char msg[] = "JSON support not yet implemented.";
      conformance_ConformanceResponse_set_skipped(
          response, upb_stringview_make(msg, sizeof(msg)));
      return;
    }

    case conformance_ConformanceRequest_payload_NOT_SET:
      fprintf(stderr, "conformance_upb: Request didn't have payload.\n");
      return;
  }

  switch (conformance_ConformanceRequest_requested_output_format(request)) {
    case conformance_UNSPECIFIED:
      fprintf(stderr, "conformance_upb: Unspecified output format.\n");
      exit(1);

    case conformance_PROTOBUF: {
      size_t serialized_len;
      char *serialized = protobuf_test_messages_proto3_TestAllTypes_serialize(
          test_message, env, &serialized_len);
      if (!serialized) {
        static const char msg[] = "Error serializing.";
        conformance_ConformanceResponse_set_serialize_error(
            response, upb_stringview_make(msg, sizeof(msg)));
        return;
      }
      conformance_ConformanceResponse_set_protobuf_payload(
          response, upb_stringview_make(serialized, serialized_len));
      break;
    }

    case conformance_JSON: {
      static const char msg[] = "JSON support not yet implemented.";
      conformance_ConformanceResponse_set_skipped(
          response, upb_stringview_make(msg, sizeof(msg)));
      break;
    }

    default:
      fprintf(stderr, "conformance_upb: Unknown output format: %d\n",
              conformance_ConformanceRequest_requested_output_format(request));
      exit(1);
  }

  return;
}

bool DoTestIo() {
  upb_env env;
  upb_status status;
  char *serialized_input;
  char *serialized_output;
  uint32_t input_size;
  size_t output_size;
  conformance_ConformanceRequest *request;
  conformance_ConformanceResponse *response;

  if (!CheckedRead(STDIN_FILENO, &input_size, sizeof(uint32_t))) {
    // EOF.
    return false;
  }

  upb_env_init(&env);
  upb_env_reporterrorsto(&env, &status);
  serialized_input = upb_env_malloc(&env, input_size);

  if (!CheckedRead(STDIN_FILENO, serialized_input, input_size)) {
    fprintf(stderr, "conformance_upb: unexpected EOF on stdin.\n");
    exit(1);
  }

  request = conformance_ConformanceRequest_parsenew(
      upb_stringview_make(serialized_input, input_size), &env);
  response = conformance_ConformanceResponse_new(&env);

  if (request) {
    DoTest(request, response, &env);
  } else {
    fprintf(stderr, "conformance_upb: parse of ConformanceRequest failed: %s\n",
            upb_status_errmsg(&status));
  }

  serialized_output = conformance_ConformanceResponse_serialize(
      response, &env, &output_size);

  CheckedWrite(STDOUT_FILENO, &output_size, sizeof(uint32_t));
  CheckedWrite(STDOUT_FILENO, serialized_output, output_size);

  test_count++;

  return true;
}

int main() {
  while (1) {
    if (!DoTestIo()) {
      fprintf(stderr, "conformance_upb: received EOF from test runner "
                      "after %d tests, exiting\n", test_count);
      return 0;
    }
  }
}
