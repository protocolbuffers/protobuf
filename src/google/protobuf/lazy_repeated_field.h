// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_LAZY_REPEATED_FIELD_H__
#define GOOGLE_PROTOBUF_LAZY_REPEATED_FIELD_H__

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/wire_format_verify.h"

// TODO: remove these
#include "base/once.h"
#include "base/spinlock_wait.h"
#include "util/gtl/no_destructor.h"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

// must be last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Descriptor;
namespace io {
class CodedInputStream;
class CodedOutputStream;
}  // namespace io
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace internal {

template <typename TagType>
inline const char* ReadTagInternal(const char* ptr, TagType* tag);

template <>
inline const char* ReadTagInternal(const char* ptr, uint8_t* tag) {
  *tag = UnalignedLoad<uint8_t>(ptr);
  return ptr + sizeof(uint8_t);
}

template <>
inline const char* ReadTagInternal(const char* ptr, uint16_t* tag) {
  *tag = UnalignedLoad<uint16_t>(ptr);
  return ptr + sizeof(uint16_t);
}

template <>
inline const char* ReadTagInternal(const char* ptr, uint32_t* tag) {
  return ReadTag(ptr, tag);
}

template <typename TagType>
inline size_t TagSizeInternal(TagType tag);
template <>
inline size_t TagSizeInternal(uint8_t tag) {
  return sizeof(uint8_t);
}
template <>
inline size_t TagSizeInternal(uint16_t tag) {
  return sizeof(uint16_t);
}
template <>
inline size_t TagSizeInternal(uint32_t tag) {
  return io::CodedOutputStream::VarintSize32(tag);
}

class LazyRepeatedPtrField {
 public:
  constexpr LazyRepeatedPtrField()
      : raw_(MessageState(RawState::kCleared)), size_(0) {}
  LazyRepeatedPtrField(const LazyField& rhs)
      : LazyRepeatedPtrField(nullptr, rhs) {}

  // Arena enabled constructors.
  LazyRepeatedPtrField(internal::InternalVisibility, Arena* arena)
      : LazyRepeatedPtrField(arena) {}
  LazyRepeatedPtrField(internal::InternalVisibility, Arena* arena,
                       const LazyRepeatedPtrField& rhs)
      : LazyRepeatedPtrField(arena, rhs) {}

  // TODO: make this constructor private
  explicit constexpr LazyRepeatedPtrField(Arena*)
      : raw_(MessageState(RawState::kCleared)), size_(0) {}

  LazyRepeatedPtrField& operator=(const LazyRepeatedPtrField&) = delete;

  ~LazyRepeatedPtrField();

  void ArenaDtor();

  // Registers this LazyRepeatedPtrField contents for arena destruction.
  void RegisterArenaDtor(Arena* arena);

  int size() const {
    switch (GetLogicalState()) {
      case LogicalState::kClear:
      case LogicalState::kClearExposed:
      case LogicalState::kNoParseRequired:
      case LogicalState::kParseRequired:
        return size_;
      case LogicalState::kDirty:
        const auto* value = raw_.load(std::memory_order_relaxed).value();
        return value->size();
    }
    // Required for certain compiler configurations.
    ABSL_LOG(FATAL) << "Not reachable";
    return -1;
  }

  // TODO: rename.
  bool IsGetEncodedTrivial() const;

  // Get and Mutable trigger parsing.
  template <typename Element>
  const RepeatedPtrField<Element>& Get(const Element* default_instance,
                                       Arena* arena) const {
    return *reinterpret_cast<const RepeatedPtrField<Element>*>(GetByPrototype(
        reinterpret_cast<const MessageLite*>(default_instance), arena));
  }

  template <typename Element>
  RepeatedPtrField<Element>* Mutable(const Element* default_instance,
                                     Arena* arena) {
    return reinterpret_cast<RepeatedPtrField<Element>*>(MutableByPrototype(
        reinterpret_cast<const MessageLite*>(default_instance), arena));
  }

  bool IsInitialized(const MessageLite* prototype, Arena* arena) const {
    switch (GetLogicalState()) {
      case LogicalState::kClear:
      case LogicalState::kClearExposed: {
        return true;
      }
      case LogicalState::kParseRequired:
      case LogicalState::kNoParseRequired: {
        // Returns true if "unparsed" is not verified to be (maybe)
        // uninitialized. Otherwise, falls through to next cases to eagerly
        // parse message and call IsInitialized().
        if (!MaybeUninitialized()) return true;
      }
        ABSL_FALLTHROUGH_INTENDED;
      case LogicalState::kDirty: {
        const auto& value = *GetByPrototype(prototype, arena);
        for (int i = value.size() - 1; i >= 0; i--) {
          if (!value.Get<GenericTypeHandler<MessageLite>>(i).IsInitialized())
            return false;
        }
        return true;
      }
      default:
        __builtin_unreachable();
    }
  }

  // Dynamic versions of basic accessors.
  const RepeatedPtrFieldBase* GetDynamic(const Descriptor* type,
                                         MessageFactory* factory,
                                         Arena* arena) const;
  RepeatedPtrFieldBase* MutableDynamic(const Descriptor* type,
                                       MessageFactory* factory, Arena* arena);

  // Basic accessors that use a default instance to create the message.
  const RepeatedPtrFieldBase* GetByPrototype(const MessageLite* prototype,
                                             Arena* arena,
                                             ParseContext* ctx = nullptr) const;
  RepeatedPtrFieldBase* MutableByPrototype(const MessageLite* prototype,
                                           Arena* arena,
                                           ParseContext* ctx = nullptr);

  void Clear();

  // Updates state such that state set in other overwrites this.
  //
  // Internal Lazy state transitions are updated as such:
  //
  // src\dest | UNINIT | INIT  | DIRTY | CLEAR         | ERROR
  // :------- | :----: | :---: | :---: | :-----------: | :---:
  //   UNINIT | DIRTY  | DIRTY | DIRTY | UNINIT/DIRTY* | DIRTY
  //   INIT   | DIRTY  | DIRTY | DIRTY | UNINIT/DIRTY* | UNDEF
  //   DIRTY  | DIRTY  | DIRTY | DIRTY | UNINIT/DIRTY* | UNDEF
  //   CLEAR  | UNINIT | INIT  | DIRTY | CLEAR         | UNDEF
  //   ERROR  | DIRTY  | DIRTY | DIRTY | DIRTY         | DIRTY
  // * Depends on if clear was initialized before.
  void MergeFrom(const MessageLite* prototype,
                 const LazyRepeatedPtrField& other, Arena* arena);

  static void Swap(LazyRepeatedPtrField* lhs, Arena* lhs_arena,
                   LazyRepeatedPtrField* rhs, Arena* rhs_arena);
  static void InternalSwap(LazyRepeatedPtrField* lhs,
                           LazyRepeatedPtrField* rhs);

  // Returns true if parsing has been attempted and it failed.
  bool HasParsingError() const {
    auto raw = raw_.load(std::memory_order_relaxed);
    return raw.status() == RawState::kParseError;
  }

  // APIs that will be used by table-driven parsing.
  template <typename TagType>
  const char* _InternalParse(const MessageLite* prototype, Arena* arena,
                             const char* ptr, ParseContext* ctx,
                             TagType expected_tag) {
    // If this message is eagerly-verified lazy, kEager mode likely suggests
    // that previous verification has failed and we fall back to eager-parsing
    // (either to initialize the message to match eager field or to fix false
    // errors.
    //
    // Lazy parsing does not support aliasing and may result in data copying.
    // It seems prudent to honor aliasing to avoid any observable gaps between
    // lazy and eager parsing.
    if (ctx->lazy_parse_mode() == ParseContext::kEager ||
        ctx->AliasingEnabled()) {
      auto* value = MutableByPrototype(prototype, arena, ctx);
      ptr = ParseToRepeatedMessage<TagType>(ptr, ctx, prototype, expected_tag,
                                            value);
      size_ = value->size();
      return ptr;
    }

    switch (GetLogicalState()) {
      case LogicalState::kParseRequired: {
        absl::Cord partial_unparsed;
        int partial_size = 0;
        ptr = ParseToCord<TagType>(ptr, ctx, prototype, expected_tag,
                                   &partial_unparsed, &partial_size);
        return _InternalParseVerify<TagType>(prototype, arena, ptr, ctx,
                                             expected_tag, &partial_unparsed,
                                             partial_size);
      } break;

      case LogicalState::kClear: {
        // Clear/Fresh have empty unparsed data; so this is the equivalent
        // of setting it to the passed in bytes.
        ABSL_DCHECK_EQ(size_, 0);
        ptr = ParseToCord<TagType>(ptr, ctx, prototype, expected_tag,
                                   &unparsed_, &size_);
        SetNeedsParse(std::move(unparsed_));
        if (ctx->parent_missing_required_fields()) {
          SetNeedsParseMaybeUninitialized();
        }
        return _InternalParseVerify<TagType>(prototype, arena, ptr, ctx,
                                             expected_tag, nullptr, 0);
      } break;

        // Pointers exposed.
      case LogicalState::kClearExposed:
      case LogicalState::kNoParseRequired:
      case LogicalState::kDirty: {
        PerformTransition([&](ExclusiveTxn& txn) {
          auto* value = txn.mutable_value();
          ptr = ParseToRepeatedMessage<TagType>(ptr, ctx, prototype,
                                                expected_tag, value);
          size_ = value->size();
          return RawState::kIsParsed;
        });
        return ptr;
      }
    }
    // Required for certain compiler configurations.
    ABSL_LOG(FATAL) << "Not reachable";
    return nullptr;
  }

  template <typename TagType>
  const char* _InternalParseVerify(const MessageLite* prototype, Arena* arena,
                                   const char* ptr, ParseContext* ctx,
                                   TagType expected_tag, absl::Cord* extra_cord,
                                   int extra_size) {
    if (ctx->lazy_parse_mode() == ParseContext::kLazy ||
        ctx->lazy_eager_verify_func() == nullptr || ptr == nullptr) {
      return ptr;
    }
    VerifyResult res = extra_cord != nullptr
                           ? WireFormatVerifyCord(*extra_cord, ctx)
                           : WireFormatVerifyCord(unparsed_, ctx);
    if (extra_cord != nullptr) {
      unparsed_.Append(std::move(*extra_cord));
      size_ += extra_size;
    }
    if (res.verified) {
      if (res.missing_required_fields) {
        // Unparsed data may be uninitialized and need to be parsed to be sure.
        SetNeedsParseMaybeUninitialized();
      }
      return ptr;
    }

    // Try eager parsing on potentially malformed wire in case the eager parsing
    // fixes the issue. For example, a negative int32 encoded as 5B varint can
    // be parsed correctly.
    //
    // Should preserve the old parsing mode because we don't want to
    // unnecessarily eager-parse other parts of message tree. This can be
    // especially inefficient if the eager verification results in false
    // positive errors.
    ParseContext::LazyParseMode old =
        ctx->set_lazy_parse_mode(ParseContext::kEager);
    (void)GetByPrototype(prototype, arena, ctx);

    // If eager parsing still fails, don't bother restoring the parse mode.
    if (HasParsingError()) return nullptr;

    // Unverified lazy fields may miss parsing errors on eager parsing. If it's
    // certain, just mark error and return.
    if (!ctx->treat_eager_parsing_errors_as_errors()) {
      auto raw = raw_.load(std::memory_order_relaxed);
      raw.set_status(RawState::kParseError);
      raw_.store(raw, std::memory_order_relaxed);
      ABSL_DCHECK(HasParsingError());
      return nullptr;
    }

    // We need to transition to dirty to prefer eager serialization as the
    // unparsed_ has non-canonical wire format.
    (void)MutableByPrototype(prototype, arena);

    (void)ctx->set_lazy_parse_mode(old);
    return ptr;
  }

  template <typename TagType>
  static const char* ParseToRepeatedMessage(const char* ptr, ParseContext* ctx,
                                            const MessageLite* prototype,
                                            TagType expected_tag,
                                            RepeatedPtrFieldBase* value) {
    const char* ptr2 = ptr;
    TagType next_tag;
    do {
      MessageLite* submsg = value->AddMessage(prototype);
      // ptr2 points to the start of the element's encoded length.
      ptr = ctx->ParseMessage(submsg, ptr2);
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
        if (ctx->Done(&ptr)) {
          break;
        }
      }
      ptr2 = ReadTagInternal<TagType>(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) return nullptr;
    } while (next_tag == expected_tag);
    return ptr;
  }

  template <typename TagType>
  static const char* ParseToCord(const char* ptr, ParseContext* ctx,
                                 const MessageLite* prototype,
                                 TagType expected_tag, absl::Cord* cord,
                                 int* size) {
    // ptr2 points to the start of the encoded length.
    const char* ptr2 = ptr;
    TagType next_tag;
    // Move ptr back to the start of the tag.
    size_t tag_size = TagSizeInternal<TagType>(expected_tag);
    ptr -= tag_size;
    do {
      // Append the tag.
      cord->Append(absl::string_view(ptr, ptr2 - ptr));
      *size = *size + 1;
      ptr = ctx->ParseLengthDelimitedInlined(
          ptr2, [ptr2, ctx, &cord](const char* ptr) {
            // At this moment length is read and ptr points to the start of
            // the payload.
            ABSL_DCHECK(ptr - ptr2 > 0 && ptr - ptr2 <= 5) << ptr - ptr2;
            cord->Append(absl::string_view(ptr2, ptr - ptr2));
            return ctx->AppendCord(ptr, cord);
          });
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
        // `Done` advances the stream to the next buffer chunk.
        if (ctx->Done(&ptr)) {
          break;
        }
      }
      // ptr points to the start of the next tag.
      ptr2 = ReadTagInternal(ptr, &next_tag);
      // ptr2 points to the start of the next element's encoded length.
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) return nullptr;
    } while (next_tag == expected_tag);
    return ptr;
  }

  uint8_t* InternalWrite(const MessageLite* prototype, int32_t number,
                         uint8_t* target,
                         io::EpsCopyOutputStream* stream) const;

  size_t ByteSizeLong(size_t tag_size) const;
  size_t SpaceUsedExcludingSelfLong() const;

  // LogicalState combines the `raw_` and `unparsed_` fields to produce the
  // current state.
  //
  // This separation allows more easily adding fine-grained states w/o touching
  // std::atomics; most state transitions are in a write context and do not
  // require subtle atomicity.
  enum class LogicalState {
    // The serialized data is available and unparsed.
    // (kParseRequired, !unparsed.empty(), message = undefined).
    kParseRequired,
    // The message has been parsed from the serialized data.
    // (kIsParsed, !unparsed.empty(), message != nullptr).
    kNoParseRequired,
    // The field is clear (freshly constructed or cleared):
    // - (kCleared, unparsed.empty(), message = nullptr)
    kClear,
    // The field is clear but previously exposed a pointer.
    // - (kCleared, unparsed.empty(), message = !nullptr)
    kClearExposed,
    // A write operation was done after a parse.
    // (kIsParsed, unparsed.empty(), message != nullptr)
    kDirty,
  };
  LogicalState GetLogicalState() const {
    // TODO: Change all CHECKs to DCHECKs
    auto raw = raw_.load(std::memory_order_acquire);
    switch (raw.status()) {
      case RawState::kParseError:
        ABSL_CHECK_NE(raw.value(), nullptr);
        return LogicalState::kDirty;
      case RawState::kCleared:
        ABSL_CHECK(unparsed_.empty());
        ABSL_CHECK(raw.value() == nullptr || raw.value()->empty());
        return raw.value() == nullptr ? LogicalState::kClear
                                      : LogicalState::kClearExposed;
      case RawState::kNeedsParse:
      case RawState::kNeedsParseMaybeUninitialized:
        // There is no SetEncoded, so unparsed_ is always from _InternalParse,
        // which can't be empty.
        ABSL_CHECK(!unparsed_.empty());
        ABSL_CHECK(raw.value() == nullptr || raw.value()->empty());
        return LogicalState::kParseRequired;
      default:
        ABSL_CHECK(raw.status() == RawState::kIsParsed ||
                   raw.status() == RawState::kIsParsedMaybeUninitialized);
        ABSL_CHECK(raw.value() != nullptr);
        // Only other Initialized state was kParseError which is handled above.
        if (unparsed_.empty()) {
          return LogicalState::kDirty;
        }
        // Non-null message, unparsed exists.
        return LogicalState::kNoParseRequired;
    }
  }

 private:
  // Values that can be kept in `MessageState`'s status bits.
  enum class RawState {
    // `unparsed_` contains the field data.
    // `message_` is either nullptr or an empty container.
    kNeedsParse,

    // kNeedsParse and may be uninitialized.
    //
    // MaybeUninitialized is flagged in the verification and recorded to trigger
    // eager parsing on IsInitialized() to be certain.
    //
    // Note that unverified data is assumed to be initialized (to support legacy
    // cases) and treated as if it's verified to be initialized. Therefore, we
    // need "MaybeUninitialized" rather than "Initialized".
    kNeedsParseMaybeUninitialized,

    // `unparsed_` contains the canonical field data.
    // `message_` points to the result of parsing that data.
    //
    // NOTE: serializing `message_` may produce different bytes than
    // `unparsed_`, so care must be taken around issues of canonical or
    // deterministic serialization.  Generally, `unparsed_` should be preferred
    // if it is not empty, as that is lower overhead.
    kIsParsed,

    // IsParsed and may be uninitialized. See
    // kNeedsParseMaybeUninitialized for details.
    kIsParsedMaybeUninitialized,

    // `unparsed_` is empty.
    // `message_` is either nullptr or an empty container.
    kCleared,

    // `message_` points to the result of parsing that data, but there was an
    // error when parsing. Partially parsed `message_` is considered canonical
    // to match eager fields.
    kParseError
  };

  class MessageState {
   public:
    constexpr explicit MessageState(RawState state) : raw_(ToUint32(state)) {}
    MessageState(const RepeatedPtrFieldBase* message, RawState state)
        : raw_(reinterpret_cast<uintptr_t>(message) | ToUint32(state)) {
      ABSL_DCHECK_EQ(reinterpret_cast<uintptr_t>(message) & ToUint32(state),
                     0u);
    }

    const RepeatedPtrFieldBase* value() const {
      return reinterpret_cast<const RepeatedPtrFieldBase*>(raw_ & ~0b111);
    }

    RepeatedPtrFieldBase* mutable_value() const {
      return reinterpret_cast<RepeatedPtrFieldBase*>(raw_ & ~0b111);
    }

    RawState status() const { return ToRawState(raw_ & 0b111); }

    void set_status(RawState status) {
      raw_ &= ~0b111;
      raw_ |= ToUint32(status);
    }

    void set_value(const RepeatedPtrFieldBase* message) {
      raw_ &= 0b111;
      raw_ |= reinterpret_cast<uintptr_t>(message);
    }

    bool NeedsParse() const {
      return status() <= RawState::kNeedsParseMaybeUninitialized;
    }

    static inline constexpr uint32_t ToUint32(RawState status) {
      return static_cast<uint32_t>(status);
    }
    static inline RawState ToRawState(uint32_t status) {
      ABSL_DCHECK_LE(status, ToUint32(RawState::kParseError));
      return static_cast<RawState>(status);
    }

   private:
    uintptr_t raw_;
  };

  LazyRepeatedPtrField(Arena* arena, const LazyRepeatedPtrField& rhs);

  // Serialization methods. Note that WriteToCord may override/clear the
  // given cord.
  bool MergeFromCord(const MessageLite* prototype, const absl::Cord& data,
                     int size, Arena* arena);

 public:
  static bool ParseWithOuterContext(RepeatedPtrFieldBase* value,
                                    const absl::Cord& cord, ParseContext* ctx,
                                    const MessageLite* prototype,
                                    bool set_missing_required);

 private:
  MessageState DoParse(RepeatedPtrFieldBase* old, const MessageLite& prototype,
                       Arena* arena, ParseContext* ctx,
                       bool maybe_uninitialized) const {
    auto* value = (old == nullptr)
                      ? Arena::CreateMessage<RepeatedPtrFieldBase>(arena)
                      : old;
    value->Reserve(size_);
    if (!ParseWithOuterContext(value, unparsed_, ctx, &prototype,
                               /*set_missing_required=*/maybe_uninitialized)) {
      // If this is called by eager verficiation, ctx != nullptr and logging
      // parsing error in that case is likely redundant because the parsing will
      // fail anyway. Users who care about parsing errors would have already
      // checked the return value and others may find the error log unexpected.
      //
      // `ctx == nullptr` means it's not eagerly verified (e.g. unverified lazy)
      // and logging in that case makes sense.
      if (ctx == nullptr) {
        LogParseError(value);
      }
      return MessageState(value, RawState::kParseError);
    }
    ABSL_DCHECK_EQ(value->size(), size_);
    return MessageState(value, maybe_uninitialized
                                   ? RawState::kIsParsedMaybeUninitialized
                                   : RawState::kIsParsed);
  }

  template <typename Strategy>
  MessageState SharedInit(Strategy strategy, Arena* arena,
                          ParseContext* ctx) const {
    auto old_raw = raw_.load(std::memory_order_acquire);
    if (!old_raw.NeedsParse() && old_raw.value() != nullptr) return old_raw;
    MessageState new_raw =
        old_raw.NeedsParse()
            ?
            // Transfer MaybeUninitialized state after a state transition.
            DoParse(nullptr, strategy.Default(), arena, ctx,
                    old_raw.status() == RawState::kNeedsParseMaybeUninitialized)
            // Otherwise, no need to parse but need to initialize the value.
            : MessageState(Arena::CreateMessage<RepeatedPtrFieldBase>(arena),
                           RawState::kCleared);
    if (raw_.compare_exchange_strong(old_raw, new_raw,
                                     std::memory_order_release,
                                     std::memory_order_acquire)) {
      // We won the race.  Dispose of the old message (if there was one).
      if (arena == nullptr) {
        delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(
            old_raw.value());
      }
      return new_raw;
    } else {
      // We lost the race, but someone else will have installed the new
      // value.  Dispose of the our attempt at installing.
      if (arena == nullptr) {
        delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(
            new_raw.value());
      }
      ABSL_DCHECK(!old_raw.NeedsParse());
      return old_raw;
    }
  }

  template <typename Strategy>
  MessageState ExclusiveInitWithoutStore(Strategy strategy, Arena* arena,
                                         ParseContext* ctx) {
    auto old_raw = raw_.load(std::memory_order_relaxed);
    if (!old_raw.NeedsParse() && old_raw.value() != nullptr) return old_raw;
    if (old_raw.NeedsParse()) {
      // Mutable messages need not transfer MaybeUninitialized.
      return DoParse(old_raw.mutable_value(), strategy.Default(), arena, ctx,
                     false);
    }
    ABSL_DCHECK(old_raw.value() == nullptr);
    return MessageState(Arena::CreateMessage<RepeatedPtrFieldBase>(arena),
                        RawState::kIsParsed);
  }

  template <typename Strategy>
  const RepeatedPtrFieldBase* GetGeneric(Strategy strategy, Arena* arena,
                                         ParseContext* ctx) const {
    const auto* value = SharedInit(strategy, arena, ctx).value();
    ABSL_DCHECK(value != nullptr);
    return value;
  }

  template <typename Strategy>
  RepeatedPtrFieldBase* MutableGeneric(Strategy strategy, Arena* arena,
                                       ParseContext* ctx) {
    auto raw = ExclusiveInitWithoutStore(strategy, arena, ctx);
    unparsed_.Clear();
    ABSL_DCHECK(raw.value() != nullptr);
    raw.set_status(RawState::kIsParsed);
    raw_.store(raw, std::memory_order_relaxed);
    return raw.mutable_value();
  }

  void SetNeedsParse(absl::Cord unparsed) {
    unparsed_ = std::move(unparsed);
    auto raw = raw_.load(std::memory_order_relaxed);
    raw.set_status(RawState::kNeedsParse);
    raw_.store(raw, std::memory_order_relaxed);
  }

  void SetNeedsParseMaybeUninitialized() {
    auto raw = raw_.load(std::memory_order_relaxed);
    ABSL_DCHECK(raw.status() == RawState::kNeedsParse ||
                raw.status() == RawState::kNeedsParseMaybeUninitialized);
    raw.set_status(RawState::kNeedsParseMaybeUninitialized);
    raw_.store(raw, std::memory_order_relaxed);
  }

  void SetParseNotRequiredMaybeUnitialized() {
    auto raw = raw_.load(std::memory_order_relaxed);
    ABSL_DCHECK(raw.status() == RawState::kIsParsed ||
                raw.status() == RawState::kIsParsedMaybeUninitialized);
    raw.set_status(RawState::kIsParsedMaybeUninitialized);
    raw_.store(raw, std::memory_order_relaxed);
  }

  bool MaybeUninitialized() const {
    auto raw = raw_.load(std::memory_order_relaxed);
    if (raw.status() == RawState::kNeedsParseMaybeUninitialized) return true;

    // Make sure the logical state matches as well.
    return raw.status() == RawState::kIsParsedMaybeUninitialized &&
           GetLogicalState() == LogicalState::kNoParseRequired;
  }

  // Adds MaybeUninitialized state if "other" may be uninitialized.
  void MergeMaybeUninitializedState(const LazyRepeatedPtrField& other);

  bool IsEagerSerializeSafe(const MessageLite* prototype, int32_t number,
                            Arena* arena) const;

  static void swap_atomics(std::atomic<MessageState>& lhs,
                           std::atomic<MessageState>& rhs);

  // Helper to enforce invariants when exclusive R/M/W access is required.
  class ExclusiveTxn {
   public:
    explicit ExclusiveTxn(LazyRepeatedPtrField& lazy)
        : lazy_(lazy), state_(lazy_.raw_.load(std::memory_order_relaxed)) {}

    RepeatedPtrFieldBase* mutable_value() {
      // Any write to the message at this point should nuke unparsed_.
      lazy_.unparsed_.Clear();
      return state_.mutable_value();
    }

    void Commit(RawState new_status) {
      if (state_.status() != new_status) {
        state_.set_status(new_status);
        lazy_.raw_.store(state_, std::memory_order_relaxed);
      }
    }

   private:
    LazyRepeatedPtrField& lazy_;
    MessageState state_;
  };

  template <typename Transition>
  RawState PerformTransition(Transition fn) {
    ExclusiveTxn txn(*this);
    RawState new_state = fn(txn);
    txn.Commit(new_state);
    return new_state;
  }

  // Mutable because it is initialized lazily.
  // A MessageState is a tagged RepeatedPtrFieldBase*
  mutable std::atomic<MessageState> raw_;

  // NOT mutable because we keep the Cord around until the message changes in
  // some way.
  absl::Cord unparsed_;
  int size_;
  friend class ::google::protobuf::Arena;
  friend class ::google::protobuf::Reflection;
  friend class ExtensionSet;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;

  // Logs a parsing error.
  static void LogParseError(const RepeatedPtrFieldBase* message);

  bool IsAllocated() const {
    return raw_.load(std::memory_order_acquire).value() != nullptr;
  }

  // For testing purposes.
  friend class LazyRepeatedPtrFieldTest;
  friend class LazyRepeatedInMessageTest;
  template <typename Element>
  void OverwriteForTest(RawState status, const absl::Cord& unparsed,
                        RepeatedPtrField<Element>* value, int size,
                        Arena* arena);
};

inline LazyRepeatedPtrField::~LazyRepeatedPtrField() {
  const auto* value = raw_.load(std::memory_order_relaxed).value();
  delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(value);
}

inline void LazyRepeatedPtrField::ArenaDtor() { unparsed_.~Cord(); }

inline void LazyRepeatedPtrField::RegisterArenaDtor(Arena* arena) {
  arena->OwnDestructor(&unparsed_);
}

inline bool LazyRepeatedPtrField::IsGetEncodedTrivial() const {
  switch (GetLogicalState()) {
    case LogicalState::kParseRequired:
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
      return true;
    case LogicalState::kDirty:
      return false;
  }
  // Required for certain compiler configurations.
  ABSL_LOG(FATAL) << "Not reachable";
  return false;
}

// -------------------------------------------------------------------
// Testing stuff.

template <typename Element>
void LazyRepeatedPtrField::OverwriteForTest(RawState status,
                                            const absl::Cord& unparsed,
                                            RepeatedPtrField<Element>* value,
                                            int size, Arena* arena) {
  auto raw = raw_.load(std::memory_order_relaxed);
  if (arena == nullptr) {
    delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(raw.value());
  }
  raw.set_value(reinterpret_cast<RepeatedPtrFieldBase*>(value));
  raw.set_status(status);
  unparsed_ = unparsed;
  size_ = size;
  raw_.store(raw, std::memory_order_relaxed);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_LAZY_REPEATED_FIELD_H__
