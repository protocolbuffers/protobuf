// Copyright 2026 Google Inc. All rights reserved.
// Test for recursion limit enforcement in MessageSet parsing.

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/unknown_field_set.h>
#include <gtest/gtest.h>
#include <string>

namespace google {
namespace protobuf {
namespace {

// Construct a MessageSet-like payload with deeply nested groups.
std::string BuildNestedMessageSetPayload(int depth) {
  // MessageSet uses group type 2 (start) and type 3 (end).
  // We use UnknownFieldSet to build a chain of nested groups.
  UnknownFieldSet fields;
  UnknownFieldSet* current = &fields;
  for (int i = 0; i < depth; ++i) {
    // Add a nested group (field number 1) to the current set.
    UnknownFieldSet* nested = current->AddGroup(1);
    current = nested;
  }
  // Serialize the whole thing.
  std::string data;
  io::StringOutputStream stream(&data);
  fields.SerializeToStream(&stream);
  return data;
}

TEST(MessageSetRecursionTest, RecursionLimitEnforced) {
  const int depth = 200;  // Exceeds default limit of 100.
  std::string payload = BuildNestedMessageSetPayload(depth);
  io::ArrayInputStream input(payload.data(), payload.size());
  io::CodedInputStream coded_input(&input);
  coded_input.SetRecursionLimit(100);

  // Parse the unknown fields. This should fail with a recursion limit error,
  // not crash.
  UnknownFieldSet parsed;
  bool success = parsed.ParseFromCodedStream(&coded_input);
  EXPECT_FALSE(success);
  // Optionally, check that the error is about recursion.
  EXPECT_TRUE(coded_input.RecursionBudgetExceeded());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
