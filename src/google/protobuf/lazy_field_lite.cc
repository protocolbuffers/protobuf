// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#include <limits>
#include <memory>
#include <utility>

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/lazy_field.h"

// Must be included last.
// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

namespace {

// Returns the initial depth for a local context.
// --The default depth for lazily verified LazyField.
// --Eagerly verified LazyField has no limit (max int).
inline int GetInitDepth(LazyVerifyOption option) {
  return option == LazyVerifyOption::kLazy
             ? io::CodedInputStream::GetDefaultRecursionLimit()
             : std::numeric_limits<int>::max();
}  // namespace

template <typename T>
inline bool ParseWithOuterContextImpl(const T& input, LazyVerifyOption option,
                                      ParseContext* ctx, MessageLite* message) {
  GOOGLE_DCHECK(ctx == nullptr || !ctx->AliasingEnabled());

  // Create local context with depth.
  const char* ptr;
  ParseContext local_ctx =
      ctx != nullptr ? ctx->Spawn(&ptr, input)
                     : ParseContext(GetInitDepth(option), false, &ptr,
                                    message->GetArenaForAllocation(), input);

  if (ctx == nullptr ||
      ctx->lazy_parse_mode() == ParseContext::LazyParseMode::kEagerVerify) {
    // Unparsed data is already verified at parsing. Disable eager-verification.
    (void)local_ctx.set_lazy_parse_mode(ParseContext::LazyParseMode::kLazy);
  }

  ptr = message->_InternalParse(ptr, &local_ctx);

  if (ctx != nullptr && local_ctx.missing_required_fields()) {
    ctx->SetMissingRequiredFields();
  }

  return ptr != nullptr &&
         (local_ctx.EndedAtEndOfStream() || local_ctx.EndedAtLimit());
}

}  // namespace

namespace {
class ByPrototype {
 public:
  explicit ByPrototype(const MessageLite* prototype) : prototype_(prototype) {}

  MessageLite* New(Arena* arena) const { return prototype_->New(arena); }

  const MessageLite& Default() const { return *prototype_; }

 private:
  const MessageLite* prototype_;
};
}  // namespace

const MessageLite& LazyField::GetByPrototype(const MessageLite& prototype,
                                             Arena* arena,
                                             LazyVerifyOption option,
                                             ParseContext* ctx) const {
  return GetGeneric(ByPrototype(&prototype), arena, option, ctx);
}

MessageLite* LazyField::MutableByPrototype(const MessageLite& prototype,
                                           Arena* arena,
                                           LazyVerifyOption option,
                                           ParseContext* ctx) {
  return MutableGeneric(ByPrototype(&prototype), arena, option, ctx);
}

MessageLite* LazyField::ReleaseByPrototype(const MessageLite& prototype,
                                           Arena* arena,
                                           LazyVerifyOption option) {
  return ReleaseGeneric(ByPrototype(&prototype), arena, option);
}

MessageLite* LazyField::UnsafeArenaReleaseByPrototype(
    const MessageLite& prototype, Arena* arena, LazyVerifyOption option) {
  return UnsafeArenaReleaseGeneric(ByPrototype(&prototype), arena, option);
}

bool LazyField::ParseWithOuterContext(MessageLite* message,
                                      LazyVerifyOption option,
                                      ParseContext* ctx) const {
  absl::optional<absl::string_view> flat = unparsed_.TryFlat();
  if (flat.has_value()) {
    return ParseWithOuterContextImpl(*flat, option, ctx, message);
  }

  io::CordInputStream input(unparsed_);
  return ParseWithOuterContextImpl(&input, option, ctx, message);
}

void LazyField::LogParseError(const MessageLite* message) {
  GOOGLE_LOG_EVERY_N(INFO, 100) << "Lazy parsing failed for " << message->GetTypeName()
                         << " error=" << message->InitializationErrorString()
                         << " (N = " << COUNTER << ")";
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
