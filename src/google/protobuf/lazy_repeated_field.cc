// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/lazy_repeated_field.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/log.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"

// Must be included last.
// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {
namespace {}  // namespace

namespace {

inline const char* InternalParseRepeated(const char* ptr,
                                         ParseContext* local_ctx,
                                         RepeatedPtrFieldBase* value,
                                         const MessageLite* prototype) {
  uint32_t expected_tag;
  ptr = ReadTag(ptr, &expected_tag);
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
  // TODO: Try to optimize this. The tags and lengths are read again
  // which is a bit wasteful.
  return LazyRepeatedPtrField::ParseToRepeatedMessage<uint32_t>(
      ptr, local_ctx, prototype, expected_tag, value);
}

template <typename T>
inline bool ParseWithNullOuterContextImpl(const T& input,
                                          RepeatedPtrFieldBase* value,
                                          const MessageLite* prototype,
                                          bool set_missing_required) {
  // Null outer context means it's either already verified or unverified.
  //
  // If the payload is eagerly verified, the recursion limit was also verified
  // and we don't need to repeat that. Also, users might have used a custom
  // limit which is not known at this access.
  //
  // Unverified lazy fields may suffer from stack overflow with deeply nested
  // data. We argue that it should be better than silent data corruption.
  constexpr int kUnlimitedDepth = std::numeric_limits<int>::max();
  const char* ptr;
  ParseContext local_ctx(kUnlimitedDepth, false, &ptr, input);

  if (set_missing_required) {
    local_ctx.SetParentMissingRequiredFields();
  }
  // Unparsed data is already verified at parsing. Disable eager-verification.
  (void)local_ctx.set_lazy_parse_mode(ParseContext::LazyParseMode::kLazy);

  ptr = InternalParseRepeated(ptr, &local_ctx, value, prototype);
  return ptr != nullptr &&
         (local_ctx.EndedAtEndOfStream() || local_ctx.EndedAtLimit());
}

template <typename T>
inline bool ParseWithOuterContextImpl(const T& input, ParseContext* ctx,
                                      RepeatedPtrFieldBase* value,
                                      const MessageLite* prototype,
                                      bool set_missing_required) {
  if (ctx == nullptr) {
    return ParseWithNullOuterContextImpl(input, value, prototype,
                                         set_missing_required);
  }

  ABSL_DCHECK(!ctx->AliasingEnabled());
  // set_missing_required => ctx == nullptr
  ABSL_DCHECK(!set_missing_required);

  // Create local context with depth.
  const char* ptr;
  ParseContext local_ctx(ParseContext::kSpawn, *ctx, &ptr, input);

  if (set_missing_required) {
    local_ctx.SetParentMissingRequiredFields();
  }
  if (ctx->lazy_parse_mode() == ParseContext::LazyParseMode::kEagerVerify) {
    // Unparsed data is already verified at parsing. Disable eager-verification.
    (void)local_ctx.set_lazy_parse_mode(ParseContext::LazyParseMode::kLazy);
  }

  ptr = InternalParseRepeated(ptr, &local_ctx, value, prototype);

  if (local_ctx.missing_required_fields()) {
    ctx->SetMissingRequiredFields();
  }

  return ptr != nullptr &&
         (local_ctx.EndedAtEndOfStream() || local_ctx.EndedAtLimit());
}

class ByPrototype {
 public:
  explicit ByPrototype(const MessageLite* prototype) : prototype_(prototype) {}

  MessageLite* New(Arena* arena) const { return prototype_->New(arena); }

  const MessageLite& Default() const { return *prototype_; }

 private:
  const MessageLite* prototype_;
};
}  // namespace

const RepeatedPtrFieldBase* LazyRepeatedPtrField::GetByPrototype(
    const MessageLite* prototype, Arena* arena, ParseContext* ctx) const {
  return GetGeneric(ByPrototype(prototype), arena, ctx);
}

RepeatedPtrFieldBase* LazyRepeatedPtrField::MutableByPrototype(
    const MessageLite* prototype, Arena* arena, ParseContext* ctx) {
  return MutableGeneric(ByPrototype(prototype), arena, ctx);
}

void LazyRepeatedPtrField::Clear() {
  PerformTransition([](ExclusiveTxn& txn) {
    auto* value = txn.mutable_value();
    if (value != nullptr) value->Clear<GenericTypeHandler<MessageLite>>();
    return RawState::kCleared;
  });
}

bool LazyRepeatedPtrField::IsEagerSerializeSafe(const MessageLite* prototype,
                                                int32_t number,
                                                Arena* arena) const {
  // "prototype" may be null if it is for dynamic messages. This is ok as
  // dynamic extensions won't be lazy as they lack verify functions any way.
  if (prototype == nullptr) return false;

  for (;;) {
    switch (GetLogicalState()) {
      case LogicalState::kClear:
      case LogicalState::kClearExposed:
      case LogicalState::kDirty:
        return true;
      case LogicalState::kNoParseRequired: {
        const auto* value = raw_.load(std::memory_order_relaxed).value();
        size_t tag_size = WireFormatLite::TagSize(
            number, WireFormatLite::FieldType::TYPE_MESSAGE);
        size_t total_size = tag_size * value->size();
        for (int i = 0; i < value->size(); i++) {
          total_size += WireFormatLite::LengthDelimitedSize(
              value->Get<GenericTypeHandler<MessageLite>>(i).ByteSizeLong());
        }
        return total_size == unparsed_.Size();
      }
      case LogicalState::kParseRequired: {
        GetByPrototype(prototype, arena);
        break;  // reswitch
      }
    }
  }
  // Required for certain compiler configurations.
  ABSL_LOG(FATAL) << "Not reachable";
  return false;
}

void LazyRepeatedPtrField::swap_atomics(std::atomic<MessageState>& lhs,
                                        std::atomic<MessageState>& rhs) {
  auto l = lhs.exchange(rhs.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
  rhs.store(l, std::memory_order_relaxed);
}

void LazyRepeatedPtrField::Swap(LazyRepeatedPtrField* lhs, Arena* lhs_arena,
                                LazyRepeatedPtrField* rhs, Arena* rhs_arena) {
  static auto reallocate = [](LazyRepeatedPtrField* f, Arena* arena,
                              bool cleanup_old) {
    auto raw = f->raw_.load(std::memory_order_relaxed);
    if (raw.value() != nullptr) {
      auto* new_value = Arena::Create<RepeatedPtrFieldBase>(arena);
      if (!raw.value()->empty()) {
        new_value->MergeFrom<MessageLite>(*raw.value());
      }
      if (cleanup_old) {
        delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(
            raw.value());
      };
      raw.set_value(new_value);
      f->raw_.store(raw, std::memory_order_relaxed);
    }
    auto old_unparsed = f->unparsed_;
    f->unparsed_.Visit(
        [] {},
        [&](auto& cord) { f->unparsed_.InitAsCord(arena, std::move(cord)); },
        [&](auto view) {
          if (arena == nullptr) {
            f->unparsed_.InitAsCord(arena, view);
          } else {
            f->unparsed_.InitAndSetArray(arena, view);
          }
        });
    if (cleanup_old) old_unparsed.Destroy();
  };
  static auto take_ownership = [](LazyRepeatedPtrField* f, Arena* arena) {
    if (internal::DebugHardenForceCopyInSwap()) {
      reallocate(f, arena, true);
    } else {
      arena->Own(reinterpret_cast<RepeatedPtrField<MessageLite>*>(
          f->raw_.load(std::memory_order_relaxed).mutable_value()));
      f->unparsed_.TransferHeapOwnershipToArena(arena);
    }
  };

  using std::swap;  // Enable ADL with fallback
  swap_atomics(lhs->raw_, rhs->raw_);
  swap(lhs->unparsed_, rhs->unparsed_);
  // At this point we are in a weird state.  The messages have been swapped into
  // their destination, but we have completely ignored the arenas, so the owning
  // arena is actually on the opposite message.  Now we straighten out our
  // ownership by forcing reallocations/ownership changes as needed.
  if (lhs_arena == rhs_arena) {
    if (internal::DebugHardenForceCopyInSwap() && lhs_arena == nullptr) {
      reallocate(lhs, lhs_arena, true);
      reallocate(rhs, rhs_arena, true);
    }
  } else {
    if (lhs_arena == nullptr) {
      take_ownership(rhs, rhs_arena);
      reallocate(lhs, lhs_arena, false);
    } else if (rhs_arena == nullptr) {
      take_ownership(lhs, lhs_arena);
      reallocate(rhs, rhs_arena, false);
    } else {
      reallocate(lhs, lhs_arena, false);
      reallocate(rhs, rhs_arena, false);
    }
  }
}

void LazyRepeatedPtrField::InternalSwap(
    LazyRepeatedPtrField* PROTOBUF_RESTRICT lhs,
    LazyRepeatedPtrField* PROTOBUF_RESTRICT rhs) {
  using std::swap;  // Enable ADL with fallback
  swap_atomics(lhs->raw_, rhs->raw_);
  swap(lhs->unparsed_, rhs->unparsed_);
}

bool LazyRepeatedPtrField::ParseWithOuterContext(RepeatedPtrFieldBase* value,
                                                 const absl::Cord& input,
                                                 ParseContext* ctx,
                                                 const MessageLite* prototype,
                                                 bool set_missing_required) {
  absl::optional<absl::string_view> flat = input.TryFlat();
  if (flat.has_value()) {
    return ParseWithOuterContextImpl(*flat, ctx, value, prototype,
                                     set_missing_required);
  }

  io::CordInputStream cis(&input);
  return ParseWithOuterContextImpl(&cis, ctx, value, prototype,
                                   set_missing_required);
}

bool LazyRepeatedPtrField::ParseWithOuterContext(RepeatedPtrFieldBase* value,
                                                 absl::string_view input,
                                                 ParseContext* ctx,
                                                 const MessageLite* prototype,
                                                 bool set_missing_required) {
  return ParseWithOuterContextImpl(input, ctx, value, prototype,
                                   set_missing_required);
}

size_t LazyRepeatedPtrField::ByteSizeLong(size_t tag_size) const {
  switch (GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kParseRequired:
      return unparsed_.Size();

    case LogicalState::kDirty:
      const auto* value = raw_.load(std::memory_order_relaxed).value();
      size_t total_size = tag_size * value->size();
      for (int i = 0; i < value->size(); i++) {
        total_size += WireFormatLite::LengthDelimitedSize(
            value->Get<GenericTypeHandler<MessageLite>>(i).ByteSizeLong());
      }
      return total_size;
  }
  // Required for certain compiler configurations.
  ABSL_LOG(FATAL) << "Not reachable";
  return -1;
}

void LazyRepeatedPtrField::LogParseError(const RepeatedPtrFieldBase* value) {
  const MessageLite* message =
      &value->at<internal::GenericTypeHandler<MessageLite>>(0);
  auto get_error_string = [&value]() {
    std::string str;
    for (int i = 0; i < value->size(); i++) {
      absl::StrAppend(&str, "[", i, "]: ",
                      value->at<internal::GenericTypeHandler<MessageLite>>(i)
                          .InitializationErrorString(),
                      "\n");
    }
    return str;
  };
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
  // In fuzzing mode, we log less to speed up fuzzing.
  ABSL_LOG_EVERY_N(INFO, 100000)
#else
  ABSL_LOG_EVERY_N_SEC(INFO, 1)
#endif
      << "Lazy parsing failed for RepeatedPtrField<" << message->GetTypeName()
      << "> error=" << get_error_string() << " (N = " << COUNTER << ")";
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
