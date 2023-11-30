// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>

#include "base/casts.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/internal_message_util.h"
#include "google/protobuf/lazy_repeated_field.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

class ByFactory {
 public:
  explicit ByFactory(const Descriptor* type, MessageFactory* factory)
      : type_(type), factory_(factory) {}

  Message* New(Arena* arena) const {
    return factory_->GetPrototype(type_)->New(arena);
  }

  const Message& Default() const { return *factory_->GetPrototype(type_); }

 private:
  const Descriptor* type_;
  MessageFactory* factory_;
};

// If LazyField is initialized, unparsed and message should be consistent. If
// a LazyField is mutated via const_cast, that may break. We should rather fail
// than silently propagate such discrepancy. Note that this aims to detect
// missing/added data.
void VerifyConsistency(LazyRepeatedPtrField::LogicalState state,
                       const RepeatedPtrFieldBase* value,
                       const MessageLite* prototype, const absl::Cord& unparsed,
                       io::EpsCopyOutputStream* stream) {
  // TODO: implement.
}

}  // namespace

LazyRepeatedPtrField::LazyRepeatedPtrField(Arena* arena,
                                           const LazyRepeatedPtrField& rhs)
    : raw_(MessageState(RawState::kCleared)), size_(rhs.size_) {
  auto new_state = RawState::kNeedsParse;
  RepeatedPtrFieldBase* value = nullptr;
  switch (rhs.GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
      return;  // Leave uninitialized / empty
    case LogicalState::kNoParseRequired:
    case LogicalState::kParseRequired:
      unparsed_ = rhs.unparsed_;
      if (rhs.MaybeUninitialized()) {
        new_state = RawState::kNeedsParseMaybeUninitialized;
      }
      break;
    case LogicalState::kDirty: {
      MessageState state = rhs.raw_.load(std::memory_order_relaxed);
      const auto* src = state.value();
      // Retain the existing IsParsed or IsParsedMaybeUninitialized status.
      // TODO: use copy construction.
      new_state = state.status();
      value = Arena::CreateMessage<RepeatedPtrFieldBase>(arena);
      value->MergeFrom<MessageLite>(*src);
    } break;
  }
  raw_.store(MessageState(value, new_state), std::memory_order_relaxed);
}

const RepeatedPtrFieldBase* LazyRepeatedPtrField::GetDynamic(
    const Descriptor* type, MessageFactory* factory, Arena* arena) const {
  return GetGeneric(ByFactory(type, factory), arena, nullptr);
}

RepeatedPtrFieldBase* LazyRepeatedPtrField::MutableDynamic(
    const Descriptor* type, MessageFactory* factory, Arena* arena) {
  return MutableGeneric(ByFactory(type, factory), arena, nullptr);
}

size_t LazyRepeatedPtrField::SpaceUsedExcludingSelfLong() const {
  // absl::Cord::EstimatedMemoryUsage counts itself that should be excluded
  // because sizeof(Cord) is already counted in self.
  size_t total_size = unparsed_.EstimatedMemoryUsage() - sizeof(absl::Cord);
  switch (GetLogicalState()) {
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kDirty: {
      const auto* value = raw_.load(std::memory_order_relaxed).value();
      total_size +=
          value->SpaceUsedExcludingSelfLong<GenericTypeHandler<Message>>();
    } break;
    case LogicalState::kClear:
    case LogicalState::kParseRequired:
      // We may have a `Message*` here, but we cannot safely access it
      // because, a racing SharedInit could delete it out from under us.
      // Other states in this structure are already passed kSharedInit and are
      // thus safe.
      break;  // Nothing to add.
  }
  return total_size;
}

bool LazyRepeatedPtrField::MergeFromCord(const MessageLite* prototype,
                                         const absl::Cord& data, int size,
                                         Arena* arena) {
  switch (GetLogicalState()) {
    case LogicalState::kParseRequired: {
      // To avoid having redundant wire, create a mutable message then merge.
      ABSL_DCHECK(prototype != nullptr);
      auto* value = MutableByPrototype(prototype, arena);
      bool res = ParseWithOuterContext(value, data, /*ctx=*/nullptr, prototype,
                                       /*set_missing_required=*/false);
      size_ = value->size();
      return res;
    }
    case LogicalState::kClear:
      SetNeedsParse(data);
      size_ = size;
      break;

    // Pointer was previously exposed merge into that object.
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kDirty: {
      auto new_state = PerformTransition([&](ExclusiveTxn& txn) {
        auto* value = txn.mutable_value();
        bool res =
            ParseWithOuterContext(value, data, /*ctx=*/nullptr, prototype,
                                  /*set_missing_required=*/false);
        size_ = value->size();
        if (!res) {
          LogParseError(value);
          return RawState::kParseError;
        } else {
          return RawState::kIsParsed;
        }
      });
      return new_state == RawState::kIsParsed;
    }
  }
  return true;
}

void LazyRepeatedPtrField::MergeMaybeUninitializedState(
    const LazyRepeatedPtrField& other) {
  if (MaybeUninitialized() || !other.MaybeUninitialized()) return;

  switch (GetLogicalState()) {
    case LogicalState::kParseRequired:
      SetNeedsParseMaybeUninitialized();
      break;
    case LogicalState::kNoParseRequired:
      SetParseNotRequiredMaybeUnitialized();
      break;
    default:
      break;
  }
}

void LazyRepeatedPtrField::MergeFrom(const MessageLite* prototype,
                                     const LazyRepeatedPtrField& other,
                                     Arena* arena) {
#ifndef NDEBUG
  // TODO: VerifyConsistency(other.GetLogicalState(),
#endif  // !NDEBUG
  switch (other.GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
      return;  // Nothing to do.

    case LogicalState::kParseRequired:
    case LogicalState::kNoParseRequired:
      MergeFromCord(prototype, other.unparsed_, other.size_, arena);
      MergeMaybeUninitializedState(other);
      return;

    case LogicalState::kDirty: {
      const auto* other_value =
          other.raw_.load(std::memory_order_relaxed).value();
      auto* value = MutableByPrototype(prototype, arena);
      value->MergeFrom<MessageLite>(*other_value);
      size_ = value->size();
      // No need to merge uninitialized state.
      ABSL_DCHECK(GetLogicalState() == LogicalState::kDirty);
      return;
    }
  }
}

uint8_t* LazyRepeatedPtrField::InternalWrite(
    const MessageLite* prototype, int32_t number, uint8_t* target,
    io::EpsCopyOutputStream* stream) const {
#ifndef NDEBUG
  VerifyConsistency(GetLogicalState(),
                    raw_.load(std::memory_order_relaxed).value(), prototype,
                    unparsed_, stream);
#endif  // !NDEBUG
  switch (GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kParseRequired:
      // If deterministic is enabled then attempt to parse to a message which
      // can then be serialized deterministically. (The serialized bytes may
      // have been created undeterministically).
      if (stream->IsSerializationDeterministic() && prototype != nullptr) {
        auto value = absl::make_unique<RepeatedPtrField<MessageLite>>();
        // TODO: Does this need to be ParsePartial?
        // TODO: Does this need to pass maybeuninit?
        // TODO: Test this path.
        bool success = ParseWithOuterContext(
            reinterpret_cast<RepeatedPtrFieldBase*>(value.get()), unparsed_,
            /*ctx=*/nullptr, prototype, false);
        if (success) {
          // TODO: alternatively, pass in the tag_size (as in ByteSizeLong).
          size_t tag_size = WireFormatLite::TagSize(
              number, WireFormatLite::FieldType::TYPE_MESSAGE);
          size_t total_size = tag_size * value->size();
          for (int i = 0; i < value->size(); i++) {
            total_size += WireFormatLite::LengthDelimitedSize(
                value->Get(i).ByteSizeLong());
          }
          if (total_size == unparsed_.size()) {
            for (unsigned i = 0, n = static_cast<unsigned>(value->size());
                 i < n; i++) {
              const auto& repfield = value->Get(i);
              target = WireFormatLite::InternalWriteMessage(
                  number, repfield, repfield.GetCachedSize(), target, stream);
            }
            return target;
          }
        }
      }
      return stream->WriteCord(unparsed_, target);
    case LogicalState::kDirty: {
      const auto* value = raw_.load(std::memory_order_relaxed).value();
      for (unsigned i = 0, n = static_cast<unsigned>(value->size()); i < n;
           i++) {
        const auto& repfield = value->Get<GenericTypeHandler<MessageLite>>(i);
        target = WireFormatLite::InternalWriteMessage(
            number, repfield, repfield.GetCachedSize(), target, stream);
      }
      return target;
    }
  }
  // Required for certain compiler configurations.
  ABSL_LOG(FATAL) << "Not reachable";
  return nullptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
