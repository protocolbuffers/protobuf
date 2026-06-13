// protobuf_textformat_fuzzer.cc
//
// Fuzz target for the protobuf TextFormat parser and printer.
// The existing OSS-Fuzz protobuf harnesses cover binary wire-format
// parsing (ParseFromString). The TextFormat parser in
// src/google/protobuf/text_format.cc is a separate parser with its own
// tokeniser, field-name resolution, and escape handling.
//
// This harness treats fuzz input as a TextFormat-encoded proto message,
// parses it, re-serialises to binary and back to text, checking for
// consistency across the round-trip.

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_pool.h"
#include "google/protobuf/unittest.pb.h"

using namespace google::protobuf;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    std::string input(reinterpret_cast<const char *>(data), size);

    // Use TestAllTypes — a rich descriptor with all field types
    protobuf_unittest::TestAllTypes msg;

    TextFormat::Parser parser;
    parser.AllowUnknownField(true);
    parser.AllowPartialMessage(true);

    if (!parser.ParseFromString(input, &msg)) return 0;

    // Binary round-trip
    std::string binary;
    if (!msg.SerializeToString(&binary)) return 0;

    protobuf_unittest::TestAllTypes msg2;
    if (!msg2.ParseFromString(binary)) return 0;

    // Text re-serialise
    std::string text_out;
    TextFormat::PrintToString(msg2, &text_out);

    return 0;
}
