// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
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

// Escape C++ trigraphs by escaping question marks to \?
std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

std::string EscapeEncoded(absl::string_view encoded) {
  std::string out;
  out.reserve(encoded.size() * 2);
  constexpr size_t kBytesPerLine = 25;
  for (size_t i = 0; i < encoded.size(); i += kBytesPerLine) {
    absl::StrAppend(
        &out, "\"",
        EscapeTrigraphs(absl::CEscape(encoded.substr(i, kBytesPerLine))),
        "\"\n");
  }
  return out;
}

// Deterministic serialization is required to minimize false positives: e.g.
// ordering, redundant wire format data, etc. Such discrepancies are
// expected and tolerated. To prevent this serialization starts yet another
// consistency check, we should skip consistency-check.
std::string DeterministicSerialization(const google::protobuf::MessageLite& m) {
  std::string result;
  {
    google::protobuf::io::StringOutputStream sink(&result);
    google::protobuf::io::CodedOutputStream out(&sink);
    out.SetSerializationDeterministic(true);
    out.SkipCheckConsistency();
    m.SerializePartialToCodedStream(&out);
  }
  return result;
}

// If LazyField is initialized, unparsed and message should be consistent. If
// a LazyField is mutated via const_cast, that may break. We should rather fail
// than silently propagate such discrepancy. Note that this aims to detect
// missing/added data.
void VerifyConsistency(LazyRepeatedPtrField::LogicalState state,
                       const RepeatedPtrFieldBase* value,
                       const MessageLite* prototype, const absl::Cord& unparsed,
                       io::EpsCopyOutputStream* stream) {
#ifndef NDEBUG
  if (stream != nullptr && !stream->ShouldCheckConsistency()) return;
  if (state != LazyRepeatedPtrField::LogicalState::kNoParseRequired) return;

  RepeatedPtrField<Message> unparsed_msg;
  if (!LazyRepeatedPtrField::ParseWithOuterContext(
          reinterpret_cast<RepeatedPtrFieldBase*>(&unparsed_msg), unparsed,
          nullptr, prototype, /*set_missing_required=*/false)) {
    // Bail out on parse failure as it can result in false positive
    // inconsistency and ABSL_CHECK failure. Warn instead.
    ABSL_LOG(WARNING)
        << "Verify skipped due to parse falure: RepeatedPtrField of "
        << prototype->GetTypeName();
    return;
  }

  const auto* msgs = reinterpret_cast<const RepeatedPtrField<Message>*>(value);
  // Eagerly parse all lazy fields to eliminate non-canonical wireformat data.
  for (int i = 0; i < msgs->size(); i++) {
    // Clone a new one from the original to eagerly parse all lazy
    // fields.
    const auto& msg = msgs->Get(i);
    std::unique_ptr<Message> clone(msg.New());
    clone->CopyFrom(msg);
    EagerParseLazyFieldIgnoreUnparsed(*clone);
    EagerParseLazyFieldIgnoreUnparsed(*unparsed_msg.Mutable(i));
    ABSL_DCHECK_EQ(DeterministicSerialization(*clone),
                   DeterministicSerialization(unparsed_msg.Get(i)))
        << "RepeatedPtrField<" << msg.GetTypeName() << ">(" << i << ")"
        << ": likely mutated via getters + const_cast\n"
        << "unparsed:\n"
        << EscapeEncoded(DeterministicSerialization(unparsed_msg.Get(i)))
        << "\nmessage:\n"
        << EscapeEncoded(DeterministicSerialization(*clone));
  }
#endif  // !NDEBUG
}

}  // namespace

LazyRepeatedPtrField::LazyRepeatedPtrField(Arena* arena,
                                           const LazyRepeatedPtrField& rhs,
                                           Arena* rhs_arena)
    : raw_(MessageState(RawState::kCleared)) {
  switch (rhs.GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
      return;  // Leave uninitialized / empty
    case LogicalState::kNoParseRequired:
    case LogicalState::kParseRequired: {
      rhs.unparsed_.Visit(
          [] {},  //
          [&](const auto& cord) { unparsed_.InitAsCord(arena, cord); },
          [&](auto view) {
            if (arena == nullptr) {
              unparsed_.InitAsCord(nullptr, view);
            } else {
              unparsed_.InitAndSetArray(arena, view);
            }
          });
      raw_.store(
          MessageState(nullptr, rhs.MaybeUninitialized()
                                    ? RawState::kNeedsParseMaybeUninitialized
                                    : RawState::kNeedsParse),
          std::memory_order_relaxed);
      return;
    }
    case LogicalState::kDirty: {
      MessageState state = rhs.raw_.load(std::memory_order_relaxed);
      const auto* src = state.value();
      if (src->empty()) {
        return;  // Leave uninitialized / empty
      }
      // Retain the existing IsParsed or IsParsedMaybeUninitialized status.
      // TODO: use copy construction.
      auto new_state = state.status();
      auto* value = Arena::Create<RepeatedPtrFieldBase>(arena);
      // MergeFrom calls reserve.
      value->MergeFrom<MessageLite>(*src);
      raw_.store(MessageState(value, new_state), std::memory_order_relaxed);
      return;
    }
  }
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
  size_t total_size = unparsed_.SpaceUsedExcludingSelf();
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

template <typename Input>
bool LazyRepeatedPtrField::MergeFrom(const MessageLite* prototype,
                                     const Input& data, Arena* arena) {
  switch (GetLogicalState()) {
    case LogicalState::kParseRequired: {
      unparsed_.UpgradeToCord(arena).Append(data);
      break;
    }
    case LogicalState::kClear: {
      size_t num_bytes = data.size();
      ABSL_DCHECK(num_bytes > 0);
      if (arena == nullptr || num_bytes > kMaxArraySize || unparsed_.IsCord()) {
        unparsed_.SetCord(arena, data);
      } else {
        unparsed_.InitAndSetArray(arena, data);
      }
      SetNeedsParse();
      break;
    }

    // Pointer was previously exposed merge into that object.
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kDirty: {
      auto new_state = PerformTransition([&](ExclusiveTxn& txn) {
        auto* value = txn.mutable_value();
        bool res =
            ParseWithOuterContext(value, data, /*ctx=*/nullptr, prototype,
                                  /*set_missing_required=*/false);
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
      SetParseNotRequiredMaybeUninitialized();
      break;
    default:
      break;
  }
}

void LazyRepeatedPtrField::MergeFrom(const MessageLite* prototype,
                                     const LazyRepeatedPtrField& other,
                                     Arena* arena, Arena* other_arena) {
#ifndef NDEBUG
  VerifyConsistency(other.GetLogicalState(),
                    other.raw_.load(std::memory_order_relaxed).value(),
                    prototype, other.unparsed_.ForceAsCord(), nullptr);
#endif  // !NDEBUG
  switch (other.GetLogicalState()) {
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
      return;  // Nothing to do.

    case LogicalState::kParseRequired:
    case LogicalState::kNoParseRequired:
      if (other.unparsed_.IsCord()) {
        MergeFrom(prototype, other.unparsed_.AsCord(), arena);
      } else {
        MergeFrom(prototype, other.unparsed_.AsStringView(), arena);
      }
      MergeMaybeUninitializedState(other);
      return;

    case LogicalState::kDirty: {
      const auto* other_value =
          other.raw_.load(std::memory_order_relaxed).value();
      if (other_value->empty()) {
        return;
      }
      auto* value = MutableByPrototype(prototype, arena);
      value->MergeFrom<MessageLite>(*other_value);
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
                    unparsed_.ForceAsCord(), stream);
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
        RepeatedPtrField<MessageLite> value;
        // TODO: Test this path.
        bool success = unparsed_.Visit(
            [] { return true; },
            [&](const auto& cord) {
              // `set_missing_required = false` to avoid checking require fields
              // (simialr to Message::ParsePartial*).
              return ParseWithOuterContext(
                  reinterpret_cast<RepeatedPtrFieldBase*>(&value), cord,
                  /*ctx=*/nullptr, prototype, /*set_missing_required=*/false);
            },
            [&](auto view) {
              return ParseWithOuterContext(
                  reinterpret_cast<RepeatedPtrFieldBase*>(&value), view,
                  /*ctx=*/nullptr, prototype, false);
            });
        if (success) {
          size_t tag_size = WireFormatLite::TagSize(
              number, WireFormatLite::FieldType::TYPE_MESSAGE);
          auto count = tag_size * value.size();
          for (int i = 0; i < value.size(); i++) {
            count += WireFormatLite::LengthDelimitedSize(
                value.Get(i).ByteSizeLong());
          }

          // Serialization takes place in two phases:
          // 1) Figure out the expected number of bytes (e.g. ByteSizeLong() on
          // the container message) 2) InternalWrite the bytes.
          //
          // There is a golden contract that the # of bytes written matches
          // the returned value from the first step.
          //
          // In this case unparsed_ was used as the source of truth for the
          // number of bytes. There are some known cases where the number of
          // serialized bytes is different than the number of bytes written
          // by a message parsed from the serialized bytes. For example if the
          // serialized representation contained multiple entries for the same
          // non-repeated field the duplicates are removed upon parsing.
          //
          // If this (relatively rare) case is hit then there is no choice
          // but to serialize the original unparsed bytes; otherwise the
          // golden contract is broken.
          // It's possible for the size to change if the unparsed_ was not
          // canonical, for example it can have repeated entries for the same
          // tag (this is more common then you would think).
          if (count == unparsed_.Size()) {
            for (int i = 0, n = value.size(); i < n; i++) {
              const auto& repfield = value.Get(i);
              target = WireFormatLite::InternalWriteMessage(
                  number, repfield, repfield.GetCachedSize(), target, stream);
            }
            return target;
          }
        }
      }
      return unparsed_.Visit(
          [&] { return target; },
          [&](const auto& cord) { return stream->WriteCord(cord, target); },
          [&](auto view) {
            return stream->WriteRaw(view.data(), view.size(), target);
          });
    case LogicalState::kDirty: {
      const auto* value = raw_.load(std::memory_order_relaxed).value();
      for (int i = 0, n = value->size(); i < n; i++) {
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
