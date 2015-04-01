// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <string>

#include "conformance.pb.h"
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/wire_format_lite.h>

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestAllTypes;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::internal::WireFormatLite;
using std::string;

int write_fd;
int read_fd;
int successes;
int failures;
bool verbose = false;

string Escape(const string& str) {
  // TODO.
  return str;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call) \
  if (call < 0) { \
    perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
    exit(1); \
  }

// TODO(haberman): make this work on Windows, instead of using these
// UNIX-specific APIs.
//
// There is a platform-agnostic API in
//    src/google/protobuf/compiler/subprocess.h
//
// However that API only supports sending a single message to the subprocess.
// We really want to be able to send messages and receive responses one at a
// time:
//
// 1. Spawning a new process for each test would take way too long for thousands
//    of tests and subprocesses like java that can take 100ms or more to start
//    up.
//
// 2. Sending all the tests in one big message and receiving all results in one
//    big message would take away our visibility about which test(s) caused a
//    crash or other fatal error.  It would also give us only a single failure
//    instead of all of them.
void SpawnTestProgram(char *executable) {
  int toproc_pipe_fd[2];
  int fromproc_pipe_fd[2];
  if (pipe(toproc_pipe_fd) < 0 || pipe(fromproc_pipe_fd) < 0) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid) {
    // Parent.
    CHECK_SYSCALL(close(toproc_pipe_fd[0]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[1]));
    write_fd = toproc_pipe_fd[1];
    read_fd = fromproc_pipe_fd[0];
  } else {
    // Child.
    CHECK_SYSCALL(close(STDIN_FILENO));
    CHECK_SYSCALL(close(STDOUT_FILENO));
    CHECK_SYSCALL(dup2(toproc_pipe_fd[0], STDIN_FILENO));
    CHECK_SYSCALL(dup2(fromproc_pipe_fd[1], STDOUT_FILENO));

    CHECK_SYSCALL(close(toproc_pipe_fd[0]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[1]));
    CHECK_SYSCALL(close(toproc_pipe_fd[1]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[0]));

    char *const argv[] = {executable, NULL};
    CHECK_SYSCALL(execv(executable, argv));  // Never returns.
  }
}

/* Invoking of tests **********************************************************/

void ReportSuccess() {
  successes++;
}

void ReportFailure(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  failures++;
}

void CheckedWrite(int fd, const void *buf, size_t len) {
  if (write(fd, buf, len) != len) {
    GOOGLE_LOG(FATAL) << "Error writing to test program: " << strerror(errno);
  }
}

void CheckedRead(int fd, void *buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    ssize_t bytes_read = read(fd, (char*)buf + ofs, len);

    if (bytes_read == 0) {
      GOOGLE_LOG(FATAL) << "Unexpected EOF from test program";
    } else if (bytes_read < 0) {
      GOOGLE_LOG(FATAL) << "Error reading from test program: " << strerror(errno);
    }

    len -= bytes_read;
    ofs += bytes_read;
  }
}

void RunTest(const ConformanceRequest& request, ConformanceResponse* response) {
  string serialized;
  request.SerializeToString(&serialized);
  uint32_t len = serialized.size();
  CheckedWrite(write_fd, &len, sizeof(uint32_t));
  CheckedWrite(write_fd, serialized.c_str(), serialized.size());
  CheckedRead(read_fd, &len, sizeof(uint32_t));
  serialized.resize(len);
  CheckedRead(read_fd, (void*)serialized.c_str(), len);
  if (!response->ParseFromString(serialized)) {
    GOOGLE_LOG(FATAL) << "Could not parse response proto from tested process.";
  }

  if (verbose) {
    fprintf(stderr, "conformance_test: request=%s, response=%s\n",
            request.ShortDebugString().c_str(),
            response->ShortDebugString().c_str());
  }
}

void DoExpectParseFailureForProto(const string& proto, int line) {
  ConformanceRequest request;
  ConformanceResponse response;
  request.set_protobuf_payload(proto);

  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  request.set_requested_output(ConformanceRequest::PROTOBUF);

  RunTest(request, &response);
  if (response.result_case() == ConformanceResponse::kParseError) {
    ReportSuccess();
  } else {
    ReportFailure("Should have failed, but didn't. Line: %d, Request: %s, "
                  "response: %s\n",
                  line,
                  request.ShortDebugString().c_str(),
                  response.ShortDebugString().c_str());
  }
}

// Expect that this precise protobuf will cause a parse error.
#define ExpectParseFailureForProto(proto) DoExpectParseFailureForProto(proto, __LINE__)

// Expect that this protobuf will cause a parse error, even if it is followed
// by valid protobuf data.  We can try running this twice: once with this
// data verbatim and once with this data followed by some valid data.
//
// TODO(haberman): implement the second of these.
#define ExpectHardParseFailureForProto(proto) DoExpectParseFailureForProto(proto, __LINE__)


/* Routines for building arbitrary protos *************************************/

// We would use CodedOutputStream except that we want more freedom to build
// arbitrary protos (even invalid ones).

const string empty;

string cat(const string& a, const string& b,
           const string& c = empty,
           const string& d = empty,
           const string& e = empty,
           const string& f = empty,
           const string& g = empty,
           const string& h = empty,
           const string& i = empty,
           const string& j = empty,
           const string& k = empty,
           const string& l = empty) {
  string ret;
  ret.reserve(a.size() + b.size() + c.size() + d.size() + e.size() + f.size() +
              g.size() + h.size() + i.size() + j.size() + k.size() + l.size());
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  ret.append(f);
  ret.append(g);
  ret.append(h);
  ret.append(i);
  ret.append(j);
  ret.append(k);
  ret.append(l);
  return ret;
}

// The maximum number of bytes that it takes to encode a 64-bit varint.
#define VARINT_MAX_LEN 10

size_t vencode64(uint64_t val, char *buf) {
  if (val == 0) { buf[0] = 0; return 1; }
  size_t i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

string varint(uint64_t x) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, buf);
  return string(buf, len);
}

// TODO: proper byte-swapping for big-endian machines.
string fixed32(void *data) { return string(static_cast<char*>(data), 4); }
string fixed64(void *data) { return string(static_cast<char*>(data), 8); }

string delim(const string& buf) { return cat(varint(buf.size()), buf); }
string uint32(uint32_t u32) { return fixed32(&u32); }
string uint64(uint64_t u64) { return fixed64(&u64); }
string flt(float f) { return fixed32(&f); }
string dbl(double d) { return fixed64(&d); }
string zz32(int32_t x) { return varint(WireFormatLite::ZigZagEncode32(x)); }
string zz64(int64_t x) { return varint(WireFormatLite::ZigZagEncode64(x)); }

string tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

string submsg(uint32_t fn, const string& buf) {
  return cat( tag(fn, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), delim(buf) );
}

#define UNKNOWN_FIELD 666

uint32_t GetFieldNumberForType(WireFormatLite::FieldType type, bool repeated) {
  const Descriptor* d = TestAllTypes().GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (static_cast<WireFormatLite::FieldType>(f->type()) == type &&
        f->is_repeated() == repeated) {
      return f->number();
    }
  }
  GOOGLE_LOG(FATAL) << "Couldn't find field with type " << (int)type;
  return 0;
}

void TestPrematureEOFForType(WireFormatLite::FieldType type) {
  // Incomplete values for each wire type.
  static const string incompletes[6] = {
    string("\x80"),     // VARINT
    string("abcdefg"),  // 64BIT
    string("\x80"),     // DELIMITED (partial length)
    string(),           // START_GROUP (no value required)
    string(),           // END_GROUP (no value required)
    string("abc")       // 32BIT
  };

  uint32_t fieldnum = GetFieldNumberForType(type, false);
  uint32_t rep_fieldnum = GetFieldNumberForType(type, true);
  WireFormatLite::WireType wire_type =
      WireFormatLite::WireTypeForFieldType(type);
  const string& incomplete = incompletes[wire_type];

  // EOF before a known non-repeated value.
  ExpectParseFailureForProto(tag(fieldnum, wire_type));

  // EOF before a known repeated value.
  ExpectParseFailureForProto(tag(rep_fieldnum, wire_type));

  // EOF before an unknown value.
  ExpectParseFailureForProto(tag(UNKNOWN_FIELD, wire_type));

  // EOF inside a known non-repeated value.
  ExpectParseFailureForProto(
      cat( tag(fieldnum, wire_type), incomplete ));

  // EOF inside a known repeated value.
  ExpectParseFailureForProto(
      cat( tag(rep_fieldnum, wire_type), incomplete ));

  // EOF inside an unknown value.
  ExpectParseFailureForProto(
      cat( tag(UNKNOWN_FIELD, wire_type), incomplete ));

  if (wire_type == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    // EOF in the middle of delimited data for known non-repeated value.
    ExpectParseFailureForProto(
        cat( tag(fieldnum, wire_type), varint(1) ));

    // EOF in the middle of delimited data for known repeated value.
    ExpectParseFailureForProto(
        cat( tag(rep_fieldnum, wire_type), varint(1) ));

    // EOF in the middle of delimited data for unknown value.
    ExpectParseFailureForProto(
        cat( tag(UNKNOWN_FIELD, wire_type), varint(1) ));

    if (type == WireFormatLite::TYPE_MESSAGE) {
      // Submessage ends in the middle of a value.
      string incomplete_submsg =
          cat( tag(WireFormatLite::TYPE_INT32, WireFormatLite::WIRETYPE_VARINT),
                incompletes[WireFormatLite::WIRETYPE_VARINT] );
      ExpectHardParseFailureForProto(
          cat( tag(fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
               varint(incomplete_submsg.size()),
               incomplete_submsg ));
    }
  } else if (type != WireFormatLite::TYPE_GROUP) {
    // Non-delimited, non-group: eligible for packing.

    // Packed region ends in the middle of a value.
    ExpectHardParseFailureForProto(
        cat( tag(rep_fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
             varint(incomplete.size()),
             incomplete ));

    // EOF in the middle of packed region.
    ExpectParseFailureForProto(
        cat( tag(rep_fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
             varint(1) ));
  }
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: conformance_test <test-program>\n");
    exit(1);
  }

  SpawnTestProgram(argv[1]);

  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; i++) {
    TestPrematureEOFForType(static_cast<WireFormatLite::FieldType>(i));
  }

  fprintf(stderr, "conformance_test: completed %d tests for %s, %d successes, "
                  "%d failures.\n", successes + failures, argv[1], successes,
                                     failures);
}
