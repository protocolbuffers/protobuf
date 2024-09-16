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
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/raw_ptr.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/wire_format_verify.h"

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

inline const char* ReadTagInternal(const char* ptr, uint8_t* tag) {
  *tag = UnalignedLoad<uint8_t>(ptr);
  return ptr + sizeof(uint8_t);
}

inline const char* ReadTagInternal(const char* ptr, uint16_t* tag) {
  *tag = UnalignedLoad<uint16_t>(ptr);
  return ptr + sizeof(uint16_t);
}

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

// This class is used to represent lazily-loaded repeated message fields.
// It stores the field in a raw buffer or a Cord initially, and then parses that
// on-demand if a caller asks for the RepeatedPtrField object.
//
// As with most protobuf classes, const methods of this class are safe to call
// from multiple threads at once, but non-const methods may only be called when
// the thread has guaranteed that it has exclusive access to the field.
class LazyRepeatedPtrField {
 public:
  constexpr LazyRepeatedPtrField() : raw_(MessageState(RawState::kCleared)) {}
  LazyRepeatedPtrField(const LazyRepeatedPtrField& rhs)
      : LazyRepeatedPtrField(nullptr, rhs, nullptr) {}

  // Arena enabled constructors.
  LazyRepeatedPtrField(internal::InternalVisibility, Arena* arena)
      : LazyRepeatedPtrField(arena) {}
  LazyRepeatedPtrField(internal::InternalVisibility, Arena* arena,
                       const LazyRepeatedPtrField& rhs, Arena* rhs_arena)
      : LazyRepeatedPtrField(arena, rhs, rhs_arena) {}

  // TODO: make this constructor private
  explicit constexpr LazyRepeatedPtrField(Arena*)
      : raw_(MessageState(RawState::kCleared)) {}

  LazyRepeatedPtrField& operator=(const LazyRepeatedPtrField&) = delete;

  ~LazyRepeatedPtrField();

  bool IsClear() const {
    auto state = GetLogicalState();
    return state == LogicalState::kClear ||
           state == LogicalState::kClearExposed;
  }

  // Get and Mutable trigger parsing.
  template <typename Element>
  const RepeatedPtrField<Element>& Get(const Element* default_instance,
                                       Arena* arena) const {
    return *reinterpret_cast<const RepeatedPtrField<Element>*>(
        GetGeneric(ByTemplate<Element>(default_instance), arena, nullptr));
  }

  template <typename Element>
  RepeatedPtrField<Element>* Mutable(const Element* default_instance,
                                     Arena* arena) {
    return reinterpret_cast<RepeatedPtrField<Element>*>(
        MutableGeneric(ByTemplate<Element>(default_instance), arena, nullptr));
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
        for (int i = 0; i < value.size(); ++i) {
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
  // TODO: The state after ERROR should be DIRTY. Also need to make the
  // change for LazyField.
  void MergeFrom(const MessageLite* prototype,
                 const LazyRepeatedPtrField& other, Arena* arena,
                 Arena* other_arena);

  static void Swap(LazyRepeatedPtrField* lhs, Arena* lhs_arena,
                   LazyRepeatedPtrField* rhs, Arena* rhs_arena);
  static void InternalSwap(LazyRepeatedPtrField* lhs,
                           LazyRepeatedPtrField* rhs);

  const RepeatedPtrFieldBase* TryGetRepeated() const;

  // Returns true when the lazy field has data that have not yet parsed.
  // (i.e. parsing has been deferred) Once parsing has been attempted, this
  // returns false. Note that the LazyField object may still contain
  // the raw unparsed data with parsing errors.
  bool HasUnparsed() const {
    return GetLogicalState() == LogicalState::kParseRequired;
  }

  // Returns true if parsing has been attempted and it failed.
  bool HasParsingError() const {
    auto raw = raw_.load(std::memory_order_relaxed);
    return raw.status() == RawState::kParseError;
  }

  // APIs that will be used by table-driven parsing.
  //
  // `TagType` is passed from table-driven parser. On fast path it's uint8 or
  // uint16; on slow path it's uint32.
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
      return ptr;
    }

    switch (GetLogicalState()) {
      case LogicalState::kParseRequired: {
        return ParseToCord<TagType>(ptr, ctx, prototype, arena, expected_tag);
      } break;

      case LogicalState::kClear: {
        // Clear/Fresh have empty unparsed data; so this is the equivalent
        // of setting it to the passed in bytes.
        return ParseToCord<TagType>(ptr, ctx, prototype, arena, expected_tag);
      } break;

        // Pointers exposed.
      case LogicalState::kClearExposed:
      case LogicalState::kNoParseRequired:
      case LogicalState::kDirty: {
        PerformTransition([&](ExclusiveTxn& txn) {
          auto* value = txn.mutable_value();
          ptr = ParseToRepeatedMessage<TagType>(ptr, ctx, prototype,
                                                expected_tag, value);
          return RawState::kIsParsed;
        });
        return ptr;
      }
    }
    // Required for certain compiler configurations.
    internal::Unreachable();
    return nullptr;
  }

  template <typename TagType>
  const char* _InternalParseVerify(const MessageLite* prototype, Arena* arena,
                                   const char* ptr, ParseContext* ctx,
                                   TagType expected_tag,
                                   absl::string_view data) {
    ABSL_DCHECK(ptr != nullptr);
    if (ctx->lazy_parse_mode() == ParseContext::kLazy ||
        ctx->lazy_eager_verify_func() == nullptr) {
      return ptr;
    }
    VerifyResult res = WireFormatVerifyView(data, ctx);
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
      ptr2 = ReadTagInternal(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) return nullptr;
    } while (next_tag == expected_tag);
    return ptr;
  }

  template <typename TagType>
  const char* ParseToCord(const char* ptr, ParseContext* ctx,
                          const MessageLite* prototype, Arena* arena,
                          TagType expected_tag) {
    // ptr2 points to the start of the encoded length.
    const char* ptr2 = ptr;
    TagType next_tag;
    // Move ptr back to the start of the tag.
    size_t tag_size = TagSizeInternal<TagType>(expected_tag);
    ptr -= tag_size;
    if (ctx->parent_missing_required_fields()) {
      SetNeedsParseMaybeUninitialized();
    } else {
      SetNeedsParse();
    }
    do {
      std::string tmp;
      // Append the tag.
      tmp.append(absl::string_view(ptr, ptr2 - ptr));
      size_t taglen_size;
      ptr = ctx->ParseLengthDelimitedInlined(
          ptr2, [&tmp, &taglen_size, ctx, ptr2](const char* p) {
            // At this moment length is read and p points to the start of
            // the payload.
            ABSL_DCHECK(p - ptr2 > 0 && p - ptr2 <= 5) << p - ptr2;
            // Append the length.
            tmp.append(absl::string_view(ptr2, p - ptr2));
            taglen_size = tmp.size();
            return ctx->AppendString(p, &tmp);
          });
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
      const auto tmp_size = tmp.size();
      ABSL_DCHECK_GE(tmp_size, taglen_size);
      if (unparsed_.IsCord()) {
        unparsed_.AsCord().Append(tmp);
      } else if (arena != nullptr &&
                 unparsed_.Size() + tmp_size <= kMaxArraySize) {
        if (unparsed_.IsEmpty()) {
          unparsed_.InitAsArray(arena, 0);
        }
        unparsed_.AppendToArray(tmp);
      } else {
        unparsed_.UpgradeToCord(arena).Append(tmp);
      }
      if (tmp_size > taglen_size) {
        ptr = _InternalParseVerify<TagType>(
            prototype, arena, ptr, ctx, expected_tag,
            absl::string_view(tmp.data() + taglen_size,
                              tmp_size - taglen_size));
        if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
      }
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
        // `Done` advances the stream to the next buffer chunk.
        if (ctx->Done(&ptr)) {
          break;
        }
      }
      // ptr points to the start of the next tag.
      ptr2 = ReadTagInternal(ptr, &next_tag);
      // ptr2 points to the start of the next element's encoded length.

      // TODO: Try to remove the following condition for 8 and 16 bits
      // TagType.
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) return nullptr;
    } while (next_tag == expected_tag);
    if (unparsed_.IsArray()) {
      unparsed_.ZeroOutTailingBytes();
    }
    return ptr;
  }

  uint8_t* InternalWrite(const MessageLite* prototype, int32_t number,
                         uint8_t* target,
                         io::EpsCopyOutputStream* stream) const;

  // ByteSize of the repeated ptr field (including the varints of tags and
  // lengths).
  size_t ByteSizeLong(size_t tag_size) const;
  size_t SpaceUsedExcludingSelfLong() const;

  // LogicalState combines the `raw_` and `unparsed_` fields to produce the
  // current state.
  //
  // This separation allows more easily adding fine-grained states w/o touching
  // std::atomics; most state transitions are in a write context and do not
  // require subtle atomicity.
  // TODO: Deduplicate with LazyField.
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
    auto raw = raw_.load(std::memory_order_acquire);
    switch (raw.status()) {
      case RawState::kParseError:
        ABSL_DCHECK_NE(raw.value(), nullptr);
        return LogicalState::kDirty;
      case RawState::kCleared:
        ABSL_DCHECK(unparsed_.IsEmpty());
        ABSL_DCHECK(raw.value() == nullptr || raw.value()->empty())
            << (raw.value() == nullptr
                    ? "nullptr"
                    : absl::StrCat("non-empty:", raw.value()->size()));
        return raw.value() == nullptr ? LogicalState::kClear
                                      : LogicalState::kClearExposed;
      case RawState::kNeedsParse:
      case RawState::kNeedsParseMaybeUninitialized:
        // There is no SetEncoded, so unparsed_ is always from _InternalParse,
        // which can't be empty.
        ABSL_DCHECK(!unparsed_.IsEmpty());
        ABSL_DCHECK(raw.value() == nullptr || raw.value()->empty());
        return LogicalState::kParseRequired;
      default:
        ABSL_DCHECK(raw.status() == RawState::kIsParsed ||
                   raw.status() == RawState::kIsParsedMaybeUninitialized);
        ABSL_DCHECK(raw.value() != nullptr);
        // Only other Initialized state was kParseError which is handled above.
        if (unparsed_.IsEmpty()) {
          return LogicalState::kDirty;
        }
        // Non-null message, unparsed exists.
        return LogicalState::kNoParseRequired;
    }
  }

 private:
  // Values that can be kept in `MessageState`'s status bits.
  // TODO: Deduplicate with LazyField.
  enum class RawState {
    // `unparsed_` is empty.
    // `message_` is either nullptr or an empty container.
    kCleared,

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

    // TODO: add kIsParsedIgnoreUnparsed and
    // kIsParsedIgnoreUnparsedMaybeUninitialized.

    // `message_` points to the result of parsing that data, but there was an
    // error when parsing. Partially parsed `message_` is considered canonical
    // to match eager fields.
    kParseError,

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

    kMaxState = kNeedsParseMaybeUninitialized
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

    static inline constexpr uint32_t ToUint32(RawState status) {
      return static_cast<uint32_t>(status);
    }
    static inline RawState ToRawState(uint32_t status) {
      ABSL_DCHECK_LE(status, ToUint32(RawState::kMaxState));
      return static_cast<RawState>(status);
    }

    bool NeedsParse() const {
      // kNeedsParse and kNeedsParseMaybeUninitialized must be 0 and 1 to make
      // NeedsParse() check cheap.
      static_assert(
          RawState::kNeedsParseMaybeUninitialized == RawState::kMaxState, "");
      static_assert(ToUint32(RawState::kNeedsParseMaybeUninitialized) ==
                        ToUint32(RawState::kNeedsParse) + 1,
                    "");
      return status() >= RawState::kNeedsParse;
    }

   private:
    uintptr_t raw_;
  };

  // TODO: Deduplicate.
  template <typename MessageType>
  class ByTemplate {
   public:
    // Only `Get()` needs access to the default element, but we don't want to
    // force instantiation of `MessageType::default_instance()` because it
    // doesn't exist in all configurations.
    explicit ByTemplate() : ByTemplate(nullptr) {}
    explicit ByTemplate(const MessageType* default_instance)
        : default_instance_(default_instance) {}

    MessageLite* New(Arena* arena) const {
      return reinterpret_cast<MessageLite*>(
          Arena::DefaultConstruct<MessageType>(arena));
    }

    const MessageLite& Default() const {
      ABSL_DCHECK(default_instance_ != nullptr);
      return *reinterpret_cast<const MessageLite*>(default_instance_);
    }

   private:
    const MessageType* default_instance_;
  };

  // Copy constructor on arena.
  LazyRepeatedPtrField(Arena* arena, const LazyRepeatedPtrField& rhs,
                       Arena* rhs_arena);

  // Serialization methods. Note that WriteToCord may override/clear the
  // given cord.
  template <typename Input>
  bool MergeFrom(const MessageLite* prototype, const Input& data, Arena* arena);

 private:
  template <typename Strategy>
  MessageState SharedInit(Strategy strategy, Arena* arena,
                          ParseContext* ctx) const {
    auto old_raw = raw_.load(std::memory_order_acquire);
    if (!old_raw.NeedsParse()) return old_raw;
    MessageState new_raw =
        // Transfer MaybeUninitialized state after a state transition.
        DoParse(nullptr, strategy.Default(), arena, ctx,
                old_raw.status() == RawState::kNeedsParseMaybeUninitialized);
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
    return MessageState(Arena::Create<RepeatedPtrFieldBase>(arena),
                        RawState::kIsParsed);
  }

  template <typename Strategy>
  const RepeatedPtrFieldBase* GetGeneric(Strategy strategy, Arena* arena,
                                         ParseContext* ctx) const {
    const auto* value = SharedInit(strategy, arena, ctx).value();
    if (value == nullptr) {
      return reinterpret_cast<const RepeatedPtrFieldBase*>(DefaultRawPtr());
    }
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

  void SetNeedsParse() {
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

  void SetParseNotRequiredMaybeUninitialized() {
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

 public:
  // Payload abstraction that can hold a raw char array or a Cord depending on
  // how much data it needs to hold.
  // The caller is responsible for managing the lifetime of the payload.
  // TODO: Deduplicate with the LazyField::UnparsedPayload.
  class UnparsedPayload {
    enum Tag : uintptr_t {
      kTagEmpty = 0,
      kTagArray = 1,
      kTagCord = 2,

      kTagBits = 3,
      kRemoveMask = ~kTagBits,
    };

   public:
    using ArraySizeType = uint16_t;

    // Visit the payload and calls the respective callback. The signatures are:
    //  - () for kUnset
    //  - (Cord&) for kCord
    //  - (absl::string_view) for kArray
    //  Returns the value returned by the callback.
    template <typename UnsetF, typename CordF, typename ViewF>
    auto Visit(UnsetF unset_f, CordF cord_f, ViewF view_f) const {
      Tag t = tag();
      // Using ternary to allow for common-type implicit conversions.
      return t == kTagEmpty   ? unset_f()
             : t == kTagArray ? view_f(AsStringView())
                              : cord_f(AsCord());
    }

    Tag tag() const { return static_cast<Tag>(value_ & kTagBits); }

    bool IsCord() const {
      ABSL_DCHECK_EQ(static_cast<bool>(value_ & kTagCord),
                     static_cast<bool>(tag() == kTagCord));
      return (value_ & kTagCord) != 0u;
    }

    bool IsArray() const {
      ABSL_DCHECK_EQ(static_cast<bool>(value_ & kTagArray),
                     static_cast<bool>(tag() == kTagArray));
      return (value_ & kTagArray) != 0u;
    }

    // Requires: IsCord()
    absl::Cord& AsCord() const {
      ABSL_DCHECK(IsCord());
      return *reinterpret_cast<absl::Cord*>(value_ & kRemoveMask);
    }

    // Return the payload as Cord regardless of the existing storage.
    absl::Cord ForceAsCord() const {
      return Visit([] { return absl::Cord(); },  //
                   [](const auto& c) { return c; },
                   [](auto view) { return absl::Cord(view); });
    }

    // Similar to AsCord(), but if the payload is not already a Cord it will
    // convert it first, maintaining existing bytes.
    absl::Cord& UpgradeToCord(Arena* arena) {
      if (IsCord()) return AsCord();
      absl::Cord new_cord(AsStringView());
      return InitAsCord(arena, std::move(new_cord));
    }

    // Requires: input array is the untagged value.
    ArraySizeType GetArraySize(const char* array) const {
      ABSL_DCHECK_EQ(array, reinterpret_cast<char*>(value_ - kTagArray));
      ArraySizeType size;
      memcpy(&size, array, sizeof(size));
      return size;
    }

    void SetArraySize(void* array, ArraySizeType size) const {
      ABSL_DCHECK_EQ(array, reinterpret_cast<void*>(value_ - kTagArray));
      memcpy(array, &size, sizeof(ArraySizeType));
    }

    void SetArraySize(ArraySizeType size) const {
      void* array = reinterpret_cast<void*>(value_ - kTagArray);
      memcpy(array, &size, sizeof(ArraySizeType));
    }

    // Requires: !IsCord()
    absl::string_view AsStringView() const {
      switch (tag()) {
        case kTagEmpty:
          return {};

        case kTagArray: {
          const char* array = reinterpret_cast<char*>(value_ - kTagArray);
          auto size = GetArraySize(array);
          return absl::string_view(array + sizeof(size), size);
        }

        default:
          Unreachable();
      }
    }

    // Clear the payload. After this call `Size()==0` and `IsEmpty()==true`, but
    // it is not necessarily true that `tag()==kTagEmpty`.
    // In particular, it keeps the Cord around in case it needs to be reused.
    void Clear() {
      switch (tag()) {
        case kTagEmpty:
        case kTagArray:
          value_ = 0;
          break;
        default:
          AsCord().Clear();
          break;
      }
    }

    // Destroys allocated memory if necessary. Does not reset the object.
    void Destroy() {
      if (IsCord()) delete &AsCord();
    }

    bool IsEmpty() const {
      return Visit([] { return true; },
                   [](const auto& cord) { return cord.empty(); },
                   [](auto view) {
                     ABSL_DCHECK(!view.empty());
                     return false;
                   });
    }

    size_t Size() const {
      return Visit([] { return 0; },
                   [](const auto& cord) { return cord.size(); },
                   [](auto view) { return view.size(); });
    }

    // Sets the currently value as a Cord constructed from `args...`.
    // It will clean up the existing value if necessary.
    template <typename Arg>
    void SetCord(Arena* arena, Arg&& arg) {
      if (IsCord()) {
        // Reuse the existing cord.
        AsCord() = std::forward<Arg>(arg);
      } else {
        absl::Cord* cord =
            Arena::Create<absl::Cord>(arena, std::forward<Arg>(arg));
        value_ = reinterpret_cast<uintptr_t>(cord) | kTagCord;
      }
    }

    // Initialize the value as a Cord constructed from `args...`
    // Ignores existing value.
    template <typename... Args>
    absl::Cord& InitAsCord(Arena* arena, Args&&... args) {
      auto* cord =
          Arena::Create<absl::Cord>(arena, std::forward<Args>(args)...);
      value_ = reinterpret_cast<uintptr_t>(cord) | kTagCord;
      return *cord;
    }

    // Initialize the value as an array copied from `view`. The tailing bytes
    // are set to 0 to avoid UB.
    // Ignores existing value.
    void InitAndSetArray(Arena* arena, absl::string_view view) {
      char* array = InitAsArray(arena, view.size());
      memcpy(array, view.data(), view.size());
      if (view.size() < kMaxArraySize) {
        // Memset uninit data to avoid UB later.
        memset(array + view.size(), '\0', kMaxArraySize - view.size());
      }
      ABSL_DCHECK_EQ(view, AsStringView());
    }

    // Initialize the value as an array copied from `cord`. The tailing bytes
    // are set to 0 to avoid UB.
    // Ignores existing value.
    void InitAndSetArray(Arena* arena, const absl::Cord& cord) {
      auto size = cord.size();
      char* array = InitAsArray(arena, size);
      cord.CopyToArray(array);
      if (size < kMaxArraySize) {
        // Memset uninit data to avoid UB later.
        memset(array + size, '\0', kMaxArraySize - size);
      }
    }

    // Initialize the value as an array of size `size`. The payload bytes are
    // uninitialized.
    // Ignores existing value.
    char* InitAsArray(Arena* arena, ArraySizeType size) {
      ABSL_DCHECK(arena != nullptr);
      // Allocate max allowed capacity.
      // TODO: improve this to reduce waste when the size is small.
      void* c = arena->AllocateAligned(kMaxArraySize + sizeof(ArraySizeType));
      ABSL_DCHECK_EQ(reinterpret_cast<uintptr_t>(c) & kTagBits, uintptr_t{0});
      value_ = reinterpret_cast<uintptr_t>(c) | kTagArray;
      SetArraySize(c, size);
      return static_cast<char*>(c) + sizeof(ArraySizeType);
    }

    void AppendToArray(absl::string_view view) {
      char* array = reinterpret_cast<char*>(value_ - kTagArray);
      ArraySizeType size = GetArraySize(array);
      char* c = array + sizeof(size) + size;
      size += view.size();
      SetArraySize(array, size);
      memcpy(c, view.data(), view.size());
    }

    void ZeroOutTailingBytes() {
      char* array = reinterpret_cast<char*>(value_ - kTagArray);
      auto size = GetArraySize(array);
      if (size < kMaxArraySize) {
        memset(array + sizeof(ArraySizeType) + size, '\0',
               kMaxArraySize - size);
      }
    }

    size_t SpaceUsedExcludingSelf() const {
      return Visit(
          [] { return 0; },
          [](const auto& cord) { return cord.EstimatedMemoryUsage(); },
          [](auto view) { return kMaxArraySize + sizeof(ArraySizeType); });
    }

    void TransferHeapOwnershipToArena(Arena* arena) {
      ABSL_DCHECK(tag() == kTagCord || tag() == kTagEmpty);
      if (IsCord()) arena->Own(&AsCord());
    }

   private:
    uintptr_t value_ = 0;
  };

 public:
  static bool ParseWithOuterContext(RepeatedPtrFieldBase* value,
                                    const absl::Cord& input, ParseContext* ctx,
                                    const MessageLite* prototype,
                                    bool set_missing_required);
  static bool ParseWithOuterContext(RepeatedPtrFieldBase* value,
                                    absl::string_view input, ParseContext* ctx,
                                    const MessageLite* prototype,
                                    bool set_missing_required);

 private:
  // This method has to be below the definition of class UnparsedPayload due to
  // the call to `unparsed_.Visit`.
  // TODO: Deduplicate with LazyField.
  MessageState DoParse(RepeatedPtrFieldBase* old, const MessageLite& prototype,
                       Arena* arena, ParseContext* ctx,
                       bool maybe_uninitialized) const {
    auto* value =
        (old == nullptr) ? Arena::Create<RepeatedPtrFieldBase>(arena) : old;
    if (!unparsed_.Visit(
            [] { return true; },
            [&](const auto& cord) {
              return ParseWithOuterContext(value, cord, ctx, &prototype,
                                           maybe_uninitialized);
            },
            [&](auto view) {
              return ParseWithOuterContext(value, view, ctx, &prototype,
                                           maybe_uninitialized);
            })) {
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
    return MessageState(value, maybe_uninitialized
                                   ? RawState::kIsParsedMaybeUninitialized
                                   : RawState::kIsParsed);
  }

  // Mutable because it is initialized lazily.
  // A MessageState is a tagged RepeatedPtrFieldBase*
  mutable std::atomic<MessageState> raw_;

  // NOT mutable because we keep the payload around until the message changes in
  // some way.
  UnparsedPayload unparsed_;
  // absl::Cord will make copies on anything under this limit, so we might as
  // well do the copies into our own buffer instead.
  static constexpr size_t kMaxArraySize = 512;
  static_assert(kMaxArraySize <=
                std::numeric_limits<UnparsedPayload::ArraySizeType>::max());
  friend class ::google::protobuf::Arena;
  friend class ::google::protobuf::Reflection;
  friend class ExtensionSet;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;

  // Logs a parsing error.
  static void LogParseError(const RepeatedPtrFieldBase* value);

  bool IsAllocated() const {
    return raw_.load(std::memory_order_acquire).value() != nullptr;
  }

  // For testing purposes.
  friend class LazyRepeatedPtrFieldTest;
  friend class LazyRepeatedInMessageTest;
  template <typename Element>
  void OverwriteForTest(RawState status, const absl::Cord& unparsed,
                        RepeatedPtrField<Element>* value, Arena* arena);
};

inline LazyRepeatedPtrField::~LazyRepeatedPtrField() {
  const auto* value = raw_.load(std::memory_order_relaxed).value();
  delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(value);
  unparsed_.Destroy();
}

// TODO: Deduplicate with LazyField.
inline const RepeatedPtrFieldBase* LazyRepeatedPtrField::TryGetRepeated()
    const {
  switch (GetLogicalState()) {
    case LogicalState::kDirty:
    case LogicalState::kNoParseRequired:
    case LogicalState::kParseRequired:
      return raw_.load(std::memory_order_relaxed).value();
    case LogicalState::kClear:
    case LogicalState::kClearExposed:
      return nullptr;
  }
  internal::Unreachable();
  return nullptr;
}

// -------------------------------------------------------------------
// Testing stuff.

// It's in the header due to the template.
// TODO: Deduplicate with LazyField.
template <typename Element>
void LazyRepeatedPtrField::OverwriteForTest(RawState status,
                                            const absl::Cord& unparsed,
                                            RepeatedPtrField<Element>* value,
                                            Arena* arena) {
  auto raw = raw_.load(std::memory_order_relaxed);
  if (arena == nullptr) {
    delete reinterpret_cast<const RepeatedPtrField<MessageLite>*>(raw.value());
  }
  raw.set_value(reinterpret_cast<RepeatedPtrFieldBase*>(value));
  raw.set_status(status);
  if (!unparsed.empty()) {
    if (arena != nullptr && unparsed.size() <= kMaxArraySize) {
      unparsed_.InitAndSetArray(arena, unparsed);
    } else {
      unparsed_.SetCord(arena, unparsed);
    }
  }
  raw_.store(raw, std::memory_order_relaxed);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_LAZY_REPEATED_FIELD_H__
