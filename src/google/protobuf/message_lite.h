// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Authors: wink@google.com (Wink Saville),
//          kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Defines MessageLite, the abstract interface implemented by all (lite
// and non-lite) protocol message objects.

#ifndef GOOGLE_PROTOBUF_MESSAGE_LITE_H__
#define GOOGLE_PROTOBUF_MESSAGE_LITE_H__

#include <atomic>
#include <climits>
#include <iosfwd>
#include <string>

#include "google/protobuf/stubs/common.h"
#include "absl/base/call_once.h"
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/port.h"


// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

template <typename T>
class RepeatedPtrField;

class FastReflectionMessageMutator;
class FastReflectionStringSetter;
class Reflection;

namespace io {

class CodedInputStream;
class CodedOutputStream;
class ZeroCopyInputStream;
class ZeroCopyOutputStream;

}  // namespace io
namespace internal {

// Allow easy change to regular int on platforms where the atomic might have a
// perf impact.
//
// CachedSize is like std::atomic<int> but with some important changes:
//
// 1) CachedSize uses Get / Set rather than load / store.
// 2) CachedSize always uses relaxed ordering.
// 3) CachedSize is assignable and copy-constructible.
// 4) CachedSize has a constexpr default constructor, and a constexpr
//    constructor that takes an int argument.
// 5) If the compiler supports the __atomic_load_n / __atomic_store_n builtins,
//    then CachedSize is trivially copyable.
//
// Developed at https://godbolt.org/z/vYcx7zYs1 ; supports gcc, clang, MSVC.
class PROTOBUF_EXPORT CachedSize {
 private:
  using Scalar = int;

 public:
  constexpr CachedSize() noexcept : atom_(Scalar{}) {}
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr CachedSize(Scalar desired) noexcept : atom_(desired) {}
#if PROTOBUF_BUILTIN_ATOMIC
  constexpr CachedSize(const CachedSize& other) = default;

  Scalar Get() const noexcept {
    return __atomic_load_n(&atom_, __ATOMIC_RELAXED);
  }

  void Set(Scalar desired) noexcept {
    __atomic_store_n(&atom_, desired, __ATOMIC_RELAXED);
  }
#else
  CachedSize(const CachedSize& other) noexcept : atom_(other.Get()) {}
  CachedSize& operator=(const CachedSize& other) noexcept {
    Set(other.Get());
    return *this;
  }

  Scalar Get() const noexcept {  //
    return atom_.load(std::memory_order_relaxed);
  }

  void Set(Scalar desired) noexcept {
    atom_.store(desired, std::memory_order_relaxed);
  }
#endif

 private:
#if PROTOBUF_BUILTIN_ATOMIC
  Scalar atom_;
#else
  std::atomic<Scalar> atom_;
#endif
};

class SwapFieldHelper;

// See parse_context.h for explanation
class ParseContext;

class ExtensionSet;
class LazyField;
class RepeatedPtrFieldBase;
class TcParser;
class WireFormatLite;
class WeakFieldMap;

template <typename Type>
class GenericTypeHandler;  // defined in repeated_field.h

// We compute sizes as size_t but cache them as int.  This function converts a
// computed size to a cached size.  Since we don't proceed with serialization
// if the total size was > INT_MAX, it is not important what this function
// returns for inputs > INT_MAX.  However this case should not error or
// ABSL_CHECK-fail, because the full size_t resolution is still returned from
// ByteSizeLong() and checked against INT_MAX; we can catch the overflow
// there.
inline int ToCachedSize(size_t size) { return static_cast<int>(size); }

// We mainly calculate sizes in terms of size_t, but some functions that
// compute sizes return "int".  These int sizes are expected to always be
// positive. This function is more efficient than casting an int to size_t
// directly on 64-bit platforms because it avoids making the compiler emit a
// sign extending instruction, which we don't want and don't want to pay for.
inline size_t FromIntSize(int size) {
  // Convert to unsigned before widening so sign extension is not necessary.
  return static_cast<unsigned int>(size);
}

// For cases where a legacy function returns an integer size.  We ABSL_DCHECK()
// that the conversion will fit within an integer; if this is false then we
// are losing information.
inline int ToIntSize(size_t size) {
  ABSL_DCHECK_LE(size, static_cast<size_t>(INT_MAX));
  return static_cast<int>(size);
}

// Default empty string object. Don't use this directly. Instead, call
// GetEmptyString() to get the reference. This empty string is aligned with a
// minimum alignment of 8 bytes to match the requirement of ArenaStringPtr.
PROTOBUF_EXPORT extern ExplicitlyConstructedArenaString
    fixed_address_empty_string;


PROTOBUF_EXPORT constexpr const std::string& GetEmptyStringAlreadyInited() {
  return fixed_address_empty_string.get();
}

PROTOBUF_EXPORT size_t StringSpaceUsedExcludingSelfLong(const std::string& str);

}  // namespace internal

// Interface to light weight protocol messages.
//
// This interface is implemented by all protocol message objects.  Non-lite
// messages additionally implement the Message interface, which is a
// subclass of MessageLite.  Use MessageLite instead when you only need
// the subset of features which it supports -- namely, nothing that uses
// descriptors or reflection.  You can instruct the protocol compiler
// to generate classes which implement only MessageLite, not the full
// Message interface, by adding the following line to the .proto file:
//
//   option optimize_for = LITE_RUNTIME;
//
// This is particularly useful on resource-constrained systems where
// the full protocol buffers runtime library is too big.
//
// Note that on non-constrained systems (e.g. servers) when you need
// to link in lots of protocol definitions, a better way to reduce
// total code footprint is to use optimize_for = CODE_SIZE.  This
// will make the generated code smaller while still supporting all the
// same features (at the expense of speed).  optimize_for = LITE_RUNTIME
// is best when you only have a small number of message types linked
// into your binary, in which case the size of the protocol buffers
// runtime itself is the biggest problem.
//
// Users must not derive from this class. Only the protocol compiler and
// the internal library are allowed to create subclasses.
class PROTOBUF_EXPORT MessageLite {
 public:
  constexpr MessageLite() {}
  MessageLite(const MessageLite&) = delete;
  MessageLite& operator=(const MessageLite&) = delete;
  virtual ~MessageLite() = default;

  // Basic Operations ------------------------------------------------

  // Get the name of this message type, e.g. "foo.bar.BazProto".
  virtual std::string GetTypeName() const = 0;

  // Construct a new instance of the same type.  Ownership is passed to the
  // caller.
  MessageLite* New() const { return New(nullptr); }

  // Construct a new instance on the arena. Ownership is passed to the caller
  // if arena is a nullptr.
  virtual MessageLite* New(Arena* arena) const = 0;

  Arena* GetArena() const { return _internal_metadata_.arena(); }

  // Clear all fields of the message and set them to their default values.
  // Clear() assumes that any memory allocated to hold parts of the message
  // will likely be needed again, so the memory used may not be freed.
  // To ensure that all memory used by a Message is freed, you must delete it.
  virtual void Clear() = 0;

  // Quickly check if all required fields have values set.
  virtual bool IsInitialized() const = 0;

  // This is not implemented for Lite messages -- it just returns "(cannot
  // determine missing fields for lite message)".  However, it is implemented
  // for full messages.  See message.h.
  virtual std::string InitializationErrorString() const;

  // If |other| is the exact same class as this, calls MergeFrom(). Otherwise,
  // results are undefined (probably crash).
  virtual void CheckTypeAndMergeFrom(const MessageLite& other) = 0;

  // These methods return a human-readable summary of the message. Note that
  // since the MessageLite interface does not support reflection, there is very
  // little information that these methods can provide. They are shadowed by
  // methods of the same name on the Message interface which provide much more
  // information. The methods here are intended primarily to facilitate code
  // reuse for logic that needs to interoperate with both full and lite protos.
  //
  // The format of the returned string is subject to change, so please do not
  // assume it will remain stable over time.
  std::string DebugString() const;
  std::string ShortDebugString() const { return DebugString(); }
  // MessageLite::DebugString is already Utf8 Safe. This is to add compatibility
  // with Message.
  std::string Utf8DebugString() const { return DebugString(); }

  // Implementation of the `AbslStringify` interface. This adds `DebugString()`
  // to the sink. Do not rely on exact format.
  template <typename Sink>
  friend void AbslStringify(Sink& sink, const google::protobuf::MessageLite& msg) {
    sink.Append(msg.DebugString());
  }

  // Parsing ---------------------------------------------------------
  // Methods for parsing in protocol buffer format.  Most of these are
  // just simple wrappers around MergeFromCodedStream().  Clear() will be
  // called before merging the input.

  // Fill the message with a protocol buffer parsed from the given input
  // stream. Returns false on a read error or if the input is in the wrong
  // format.  A successful return does not indicate the entire input is
  // consumed, ensure you call ConsumedEntireMessage() to check that if
  // applicable.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromCodedStream(
      io::CodedInputStream* input);
  // Like ParseFromCodedStream(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromCodedStream(
      io::CodedInputStream* input);
  // Read a protocol buffer from the given zero-copy input stream.  If
  // successful, the entire input will be consumed.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromZeroCopyStream(
      io::ZeroCopyInputStream* input);
  // Like ParseFromZeroCopyStream(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromZeroCopyStream(
      io::ZeroCopyInputStream* input);
  // Parse a protocol buffer from a file descriptor.  If successful, the entire
  // input will be consumed.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromFileDescriptor(
      int file_descriptor);
  // Like ParseFromFileDescriptor(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromFileDescriptor(
      int file_descriptor);
  // Parse a protocol buffer from a C++ istream.  If successful, the entire
  // input will be consumed.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromIstream(std::istream* input);
  // Like ParseFromIstream(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromIstream(
      std::istream* input);
  // Read a protocol buffer from the given zero-copy input stream, expecting
  // the message to be exactly "size" bytes long.  If successful, exactly
  // this many bytes will have been consumed from the input.
  bool MergePartialFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input,
                                             int size);
  // Like ParseFromBoundedZeroCopyStream(), but accepts messages that are
  // missing required fields.
  bool MergeFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input, int size);
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromBoundedZeroCopyStream(
      io::ZeroCopyInputStream* input, int size);
  // Like ParseFromBoundedZeroCopyStream(), but accepts messages that are
  // missing required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromBoundedZeroCopyStream(
      io::ZeroCopyInputStream* input, int size);
  // Parses a protocol buffer contained in a string. Returns true on success.
  // This function takes a string in the (non-human-readable) binary wire
  // format, matching the encoding output by MessageLite::SerializeToString().
  // If you'd like to convert a human-readable string into a protocol buffer
  // object, see google::protobuf::TextFormat::ParseFromString().
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromString(absl::string_view data);
  // Like ParseFromString(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromString(
      absl::string_view data);
  // Parse a protocol buffer contained in an array of bytes.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromArray(const void* data,
                                                       int size);
  // Like ParseFromArray(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromArray(const void* data,
                                                              int size);


  // Reads a protocol buffer from the stream and merges it into this
  // Message.  Singular fields read from the what is
  // already in the Message and repeated fields are appended to those
  // already present.
  //
  // It is the responsibility of the caller to call input->LastTagWas()
  // (for groups) or input->ConsumedEntireMessage() (for non-groups) after
  // this returns to verify that the message's end was delimited correctly.
  //
  // ParseFromCodedStream() is implemented as Clear() followed by
  // MergeFromCodedStream().
  bool MergeFromCodedStream(io::CodedInputStream* input);

  // Like MergeFromCodedStream(), but succeeds even if required fields are
  // missing in the input.
  //
  // MergeFromCodedStream() is just implemented as MergePartialFromCodedStream()
  // followed by IsInitialized().
  bool MergePartialFromCodedStream(io::CodedInputStream* input);

  // Merge a protocol buffer contained in a string.
  bool MergeFromString(absl::string_view data);


  // Serialization ---------------------------------------------------
  // Methods for serializing in protocol buffer format.  Most of these
  // are just simple wrappers around ByteSize() and SerializeWithCachedSizes().

  // Write a protocol buffer of this message to the given output.  Returns
  // false on a write error.  If the message is missing required fields,
  // this may ABSL_CHECK-fail.
  bool SerializeToCodedStream(io::CodedOutputStream* output) const;
  // Like SerializeToCodedStream(), but allows missing required fields.
  bool SerializePartialToCodedStream(io::CodedOutputStream* output) const;
  // Write the message to the given zero-copy output stream.  All required
  // fields must be set.
  bool SerializeToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  // Like SerializeToZeroCopyStream(), but allows missing required fields.
  bool SerializePartialToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  // Serialize the message and store it in the given string.  All required
  // fields must be set.
  bool SerializeToString(std::string* output) const;
  // Like SerializeToString(), but allows missing required fields.
  bool SerializePartialToString(std::string* output) const;
  // Serialize the message and store it in the given byte array.  All required
  // fields must be set.
  bool SerializeToArray(void* data, int size) const;
  // Like SerializeToArray(), but allows missing required fields.
  bool SerializePartialToArray(void* data, int size) const;

  // Make a string encoding the message. Is equivalent to calling
  // SerializeToString() on a string and using that.  Returns the empty
  // string if SerializeToString() would have returned an error.
  // Note: If you intend to generate many such strings, you may
  // reduce heap fragmentation by instead re-using the same string
  // object with calls to SerializeToString().
  std::string SerializeAsString() const;
  // Like SerializeAsString(), but allows missing required fields.
  std::string SerializePartialAsString() const;

  // Serialize the message and write it to the given file descriptor.  All
  // required fields must be set.
  bool SerializeToFileDescriptor(int file_descriptor) const;
  // Like SerializeToFileDescriptor(), but allows missing required fields.
  bool SerializePartialToFileDescriptor(int file_descriptor) const;
  // Serialize the message and write it to the given C++ ostream.  All
  // required fields must be set.
  bool SerializeToOstream(std::ostream* output) const;
  // Like SerializeToOstream(), but allows missing required fields.
  bool SerializePartialToOstream(std::ostream* output) const;

  // Like SerializeToString(), but appends to the data to the string's
  // existing contents.  All required fields must be set.
  bool AppendToString(std::string* output) const;
  // Like AppendToString(), but allows missing required fields.
  bool AppendPartialToString(std::string* output) const;

  // Reads a protocol buffer from a Cord and merges it into this message.
  bool MergeFromCord(const absl::Cord& cord);
  // Like MergeFromCord(), but accepts messages that are missing
  // required fields.
  bool MergePartialFromCord(const absl::Cord& cord);
  // Parse a protocol buffer contained in a Cord.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParseFromCord(const absl::Cord& cord);
  // Like ParseFromCord(), but accepts messages that are missing
  // required fields.
  PROTOBUF_ATTRIBUTE_REINITIALIZES bool ParsePartialFromCord(
      const absl::Cord& cord);

  // Serialize the message and store it in the given Cord.  All required
  // fields must be set.
  bool SerializeToCord(absl::Cord* output) const;
  // Like SerializeToCord(), but allows missing required fields.
  bool SerializePartialToCord(absl::Cord* output) const;

  // Make a Cord encoding the message. Is equivalent to calling
  // SerializeToCord() on a Cord and using that.  Returns an empty
  // Cord if SerializeToCord() would have returned an error.
  absl::Cord SerializeAsCord() const;
  // Like SerializeAsCord(), but allows missing required fields.
  absl::Cord SerializePartialAsCord() const;

  // Like SerializeToCord(), but appends to the data to the Cord's existing
  // contents.  All required fields must be set.
  bool AppendToCord(absl::Cord* output) const;
  // Like AppendToCord(), but allows missing required fields.
  bool AppendPartialToCord(absl::Cord* output) const;

  // Computes the serialized size of the message.  This recursively calls
  // ByteSizeLong() on all embedded messages.
  //
  // ByteSizeLong() is generally linear in the number of fields defined for the
  // proto.
  virtual size_t ByteSizeLong() const = 0;

  // Legacy ByteSize() API.
  [[deprecated("Please use ByteSizeLong() instead")]] int ByteSize() const {
    return internal::ToIntSize(ByteSizeLong());
  }

  // Serializes the message without recomputing the size.  The message must not
  // have changed since the last call to ByteSize(), and the value returned by
  // ByteSize must be non-negative.  Otherwise the results are undefined.
  void SerializeWithCachedSizes(io::CodedOutputStream* output) const {
    output->SetCur(_InternalSerialize(output->Cur(), output->EpsCopy()));
  }

  // Functions below here are not part of the public interface.  It isn't
  // enforced, but they should be treated as private, and will be private
  // at some future time.  Unfortunately the implementation of the "friend"
  // keyword in GCC is broken at the moment, but we expect it will be fixed.

  // Like SerializeWithCachedSizes, but writes directly to *target, returning
  // a pointer to the byte immediately after the last byte written.  "target"
  // must point at a byte array of at least ByteSize() bytes.  Whether to use
  // deterministic serialization, e.g., maps in sorted order, is determined by
  // CodedOutputStream::IsDefaultSerializationDeterministic().
  uint8_t* SerializeWithCachedSizesToArray(uint8_t* target) const;

  // Returns the result of the last call to ByteSize().  An embedded message's
  // size is needed both to serialize it (because embedded messages are
  // length-delimited) and to compute the outer message's size.  Caching
  // the size avoids computing it multiple times.
  //
  // ByteSize() does not automatically use the cached size when available
  // because this would require invalidating it every time the message was
  // modified, which would be too hard and expensive.  (E.g. if a deeply-nested
  // sub-message is changed, all of its parents' cached sizes would need to be
  // invalidated, which is too much work for an otherwise inlined setter
  // method.)
  int GetCachedSize() const;

  virtual const char* _InternalParse(const char* /*ptr*/,
                                     internal::ParseContext* /*ctx*/) {
    return nullptr;
  }

  void OnDemandRegisterArenaDtor(Arena* arena);

 protected:
  // Message implementations require access to internally visible API.
  static constexpr internal::InternalVisibility internal_visibility() {
    return internal::InternalVisibility{};
  }

  template <typename T>
  static T* CreateMaybeMessage(Arena* arena) {
    return Arena::CreateMaybeMessage<T>(arena);
  }

#ifdef PROTOBUF_EXPLICIT_CONSTRUCTORS
  template <typename T>
  static T* CreateMaybeMessage(Arena* arena, const T& from) {
    return Arena::CreateMaybeMessage<T>(arena, from);
  }
#endif  // PROTOBUF_EXPLICIT_CONSTRUCTORS

  inline explicit MessageLite(Arena* arena) : _internal_metadata_(arena) {}

  // Returns the arena, if any, that directly owns this message and its internal
  // memory (Arena::Own is different in that the arena doesn't directly own the
  // internal memory). This method is used in proto's implementation for
  // swapping, moving and setting allocated, for deciding whether the ownership
  // of this message or its internal memory could be changed.
  Arena* GetOwningArena() const { return _internal_metadata_.arena(); }

  struct ClassData {
    // Note: The order of arguments in the functions is chosen so that it has
    // the same ABI as the member function that calls them. Eg the `this`
    // pointer becomes the first argument in the free function.
    void (*merge_to_from)(Message& to, const Message& from_msg);
    void (*on_demand_register_arena_dtor)(MessageLite& msg, Arena& arena);
  };

  // GetClassData() returns a pointer to a ClassData struct which
  // exists in global memory and is unique to each subclass.  This uniqueness
  // property is used in order to quickly determine whether two messages are
  // of the same type.
  //
  // This is a work in progress. Currently only SPEED messages return an
  // instance. In the future all message types will return one.
  virtual const ClassData* GetClassData() const;

  internal::InternalMetadata _internal_metadata_;

  // The default implementation means there is no cached size and ByteSize
  // should be called instead.
  virtual internal::CachedSize* AccessCachedSize() const;

 public:
  enum ParseFlags {
    kMerge = 0,
    kParse = 1,
    kMergePartial = 2,
    kParsePartial = 3,
    kMergeWithAliasing = 4,
    kParseWithAliasing = 5,
    kMergePartialWithAliasing = 6,
    kParsePartialWithAliasing = 7
  };

  template <ParseFlags flags, typename T>
  bool ParseFrom(const T& input);

  // Fast path when conditions match (ie. non-deterministic)
  //  uint8_t* _InternalSerialize(uint8_t* ptr) const;
  virtual uint8_t* _InternalSerialize(
      uint8_t* ptr, io::EpsCopyOutputStream* stream) const = 0;

  // Identical to IsInitialized() except that it logs an error message.
  bool IsInitializedWithErrors() const {
    if (IsInitialized()) return true;
    LogInitializationErrorMessage();
    return false;
  }

 private:
  friend class FastReflectionMessageMutator;
  friend class FastReflectionStringSetter;
  friend class Message;
  friend class Reflection;
  friend class internal::ExtensionSet;
  friend class internal::LazyField;
  friend class internal::SwapFieldHelper;
  friend class internal::TcParser;
  friend class internal::WeakFieldMap;
  friend class internal::WireFormatLite;

  template <typename Type>
  friend class Arena::InternalHelper;
  template <typename Type>
  friend class internal::GenericTypeHandler;

  void LogInitializationErrorMessage() const;

  bool MergeFromImpl(io::CodedInputStream* input, ParseFlags parse_flags);
};

namespace internal {

template <bool alias>
bool MergeFromImpl(absl::string_view input, MessageLite* msg,
                   MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<false>(absl::string_view input,
                                          MessageLite* msg,
                                          MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<true>(absl::string_view input,
                                         MessageLite* msg,
                                         MessageLite::ParseFlags parse_flags);

template <bool alias>
bool MergeFromImpl(io::ZeroCopyInputStream* input, MessageLite* msg,
                   MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<false>(io::ZeroCopyInputStream* input,
                                          MessageLite* msg,
                                          MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<true>(io::ZeroCopyInputStream* input,
                                         MessageLite* msg,
                                         MessageLite::ParseFlags parse_flags);

struct BoundedZCIS {
  io::ZeroCopyInputStream* zcis;
  int limit;
};

template <bool alias>
bool MergeFromImpl(BoundedZCIS input, MessageLite* msg,
                   MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<false>(BoundedZCIS input, MessageLite* msg,
                                          MessageLite::ParseFlags parse_flags);
extern template bool MergeFromImpl<true>(BoundedZCIS input, MessageLite* msg,
                                         MessageLite::ParseFlags parse_flags);

template <typename T>
struct SourceWrapper;

template <bool alias, typename T>
bool MergeFromImpl(const SourceWrapper<T>& input, MessageLite* msg,
                   MessageLite::ParseFlags parse_flags) {
  return input.template MergeInto<alias>(msg, parse_flags);
}

}  // namespace internal

template <MessageLite::ParseFlags flags, typename T>
bool MessageLite::ParseFrom(const T& input) {
  if (flags & kParse) Clear();
  constexpr bool alias = (flags & kMergeWithAliasing) != 0;
  return internal::MergeFromImpl<alias>(input, this, flags);
}

// ===================================================================
// Shutdown support.


// Shut down the entire protocol buffers library, deleting all static-duration
// objects allocated by the library or by generated .pb.cc files.
//
// There are two reasons you might want to call this:
// * You use a draconian definition of "memory leak" in which you expect
//   every single malloc() to have a corresponding free(), even for objects
//   which live until program exit.
// * You are writing a dynamically-loaded library which needs to clean up
//   after itself when the library is unloaded.
//
// It is safe to call this multiple times.  However, it is not safe to use
// any other part of the protocol buffers library after
// ShutdownProtobufLibrary() has been called. Furthermore this call is not
// thread safe, user needs to synchronize multiple calls.
PROTOBUF_EXPORT void ShutdownProtobufLibrary();

namespace internal {

// Register a function to be called when ShutdownProtocolBuffers() is called.
PROTOBUF_EXPORT void OnShutdown(void (*func)());
// Run an arbitrary function on an arg
PROTOBUF_EXPORT void OnShutdownRun(void (*f)(const void*), const void* arg);

template <typename T>
T* OnShutdownDelete(T* p) {
  OnShutdownRun([](const void* pp) { delete static_cast<const T*>(pp); }, p);
  return p;
}

}  // namespace internal

std::string ShortFormat(const MessageLite& message_lite);
std::string Utf8Format(const MessageLite& message_lite);

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MESSAGE_LITE_H__
