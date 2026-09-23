// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RECURSION_LIMIT_PAYLOADS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RECURSION_LIMIT_PAYLOADS_H__

#include "conformance/binary_wireformat.h"

// The deeply nested payloads of the recursion-limit performance tests
// (binary_recursion_limit_test.cc).  They are built directly on the wire
// rather than serialized from a message object of that depth: ByteSizeLong(),
// the serializer and the destructor recurse once per nesting level, which
// overflows the stack under the sanitizers' larger frames (b/563731788).  The
// bytes are identical to what the message objects serialize, which
// recursion_limit_payloads_test.cc checks at a depth the stack can take.

namespace google {
namespace protobuf {
namespace conformance {

// TestAllTypesEdition2023 whose map_recursive (301) entry {key 0, value}
// holds a TestAllTypesEdition2023 with optional_int32 (1) = 123 and, `depth`
// times in all, its own such entry.  `depth` must be at least 1.
Wire DeepMapPayload(int depth);

// TestAllTypesEdition2023 whose map_string_nested_message (71) entry {key "",
// value} holds a NestedMessage whose corecursive (2) is a
// TestAllTypesEdition2023 with optional_int32 (1) = 123 and, `depth` times in
// all, its own such entry.  `depth` must be at least 1.
Wire DeepMapStringKeyPayload(int depth);

// TestAllTypesProto2 whose message_set_correct (500) holds a MessageSetCorrect
// with one MessageSetCorrectExtension2 item (type id 4135312) whose sub_msg
// (10) is again such a MessageSetCorrect, `depth` times, the innermost
// extension holding i (9) = 123 instead (with `depth` 0, the only one).
Wire DeepMessageSetPayload(int depth);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_RECURSION_LIMIT_PAYLOADS_H__
