// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// Defines ClassData, the structure that contains type information about
// messages. Used to implement reflection, parsing, dynamic casting, and other
// runtime features.
//
// This file intentionally does not depend on protobuf_lite.

#ifndef GOOGLE_PROTOBUF_CLASS_DATA_H__
#define GOOGLE_PROTOBUF_CLASS_DATA_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Arena;
class Descriptor;
class MessageLite;
class Reflection;

namespace io {
class EpsCopyOutputStream;
}  // namespace io

namespace internal {

struct DescriptorTable;
class LazyField;
struct TcParseTableBase;
struct DescriptorMethods;

class MessageCreator {
 public:
  using Func = void* (*)(const void*, void*, Arena*);

  // Use -1/0/1 to be able to use <0, ==0, >0
  enum Tag : int8_t {
    kFunc = -1,
    kZeroInit = 0,
    kMemcpy = 1,
  };

  constexpr MessageCreator()
      : allocation_size_(), tag_(), alignment_(), func_(nullptr) {}

  static constexpr MessageCreator ZeroInit(uint32_t allocation_size,
                                           uint8_t alignment) {
    MessageCreator out;
    out.allocation_size_ = allocation_size;
    out.tag_ = kZeroInit;
    out.alignment_ = alignment;
    return out;
  }
  static constexpr MessageCreator CopyInit(uint32_t allocation_size,
                                           uint8_t alignment) {
    MessageCreator out;
    out.allocation_size_ = allocation_size;
    out.tag_ = kMemcpy;
    out.alignment_ = alignment;
    return out;
  }
  constexpr MessageCreator(Func func, uint32_t allocation_size,
                           uint8_t alignment)
      : allocation_size_(allocation_size),
        tag_(kFunc),
        alignment_(alignment),
        func_(func) {}

  // Template for testing.
  template <typename MessageLite>
  MessageLite* PlacementNew(const MessageLite* prototype_for_func,
                            const MessageLite* prototype_for_copy, void* mem,
                            Arena* arena) const;

  // Make this a template to avoid depending on arena.h.
  template <typename Arena>
  void* AllocateMessage(Arena* arena) const {
    if (arena != nullptr) {
      return arena->AllocateAligned(allocation_size_);
    } else {
      return Allocate(allocation_size_);
    }
  }

  Tag tag() const { return tag_; }

  uint32_t allocation_size() const { return allocation_size_; }

  uint8_t alignment() const { return alignment_; }

 private:
  uint32_t allocation_size_;
  Tag tag_;
  uint8_t alignment_;
  Func func_;
};

// ClassData* can and should be placed on read-only section to maximize sharing.
// However, !LITE has mutable fields for lazy initialization of reflection
// related data. We move these fields to a separate struct to keep the main
// ClassData read-only. Extra indirection should be tolerable considering that
// reflection isn't performance critical.
struct PROTOBUF_EXPORT ReflectionData {
  constexpr ReflectionData(const DescriptorMethods* descriptor_methods,
                           const internal::DescriptorTable* descriptor_table,
                           void (*get_metadata_tracker)())
      : reflection(nullptr),
        descriptor(nullptr),
        descriptor_table(descriptor_table),
        descriptor_methods(descriptor_methods),
        get_metadata_tracker(get_metadata_tracker) {}

  // Accesses are protected by the once_flag in `descriptor_table`. When the
  // table is null these are populated from the beginning and need to
  // protection.
  const Reflection* reflection;
  const Descriptor* descriptor;

  // Codegen types will provide a DescriptorTable to do lazy
  // registration/initialization of the reflection objects.
  // Other types, like DynamicMessage, keep the table as null but eagerly
  // populate `reflection`/`descriptor` fields.
  const internal::DescriptorTable* descriptor_table;
  const DescriptorMethods* descriptor_methods;
  // When an access tracker is installed, this function notifies the tracker
  // that GetMetadata was called.
  void (*get_metadata_tracker)();
};

// Note: The order of arguments in the functions is chosen so that it has
// the same ABI as the member function that calls them. Eg the `this`
// pointer becomes the first argument in the free function.
//
// Future work:
// We could save more data by omitting any optional pointer that would
// otherwise be null. We can have some metadata in ClassData telling us if we
// have them and their offset.

struct PROTOBUF_EXPORT ClassData {
  bool (*is_initialized)(const MessageLite&);
  void (*merge_to_from)(MessageLite& to, const MessageLite& from_msg);
  internal::MessageCreator message_creator;
#if defined(PROTOBUF_CUSTOM_VTABLE)
  void (*destroy_message)(MessageLite& msg);
  void (*clear)(MessageLite& msg);
  size_t (*byte_size_long)(const MessageLite&);
  uint8_t* (*serialize)(const MessageLite& msg, uint8_t* ptr,
                        io::EpsCopyOutputStream* stream);
#endif  // PROTOBUF_CUSTOM_VTABLE

  // Offset of the CachedSize member.
  uint32_t cached_size_offset;
  // LITE objects (ie !descriptor_methods) collocate their name as a
  // char[] just beyond the ClassData.
  bool is_lite;
  bool is_dynamic = false;

  // In normal mode we have the small constructor to avoid the cost in
  // codegen.
#if !defined(PROTOBUF_CUSTOM_VTABLE)
  constexpr ClassData(
      bool (*is_initialized)(const MessageLite&),
      void (*merge_to_from)(MessageLite& to, const MessageLite& from_msg),
      internal::MessageCreator message_creator, uint32_t cached_size_offset,
      std::variant<ReflectionData*, const char*> reflection_or_name)
      : ClassData(is_initialized, merge_to_from, message_creator, nullptr,
                  nullptr, nullptr, nullptr, cached_size_offset,
                  reflection_or_name) {}
#endif  // !PROTOBUF_CUSTOM_VTABLE

  // But we always provide the full constructor even in normal mode to make
  // helper code simpler.
  constexpr ClassData(
      bool (*is_initialized)(const MessageLite&),
      void (*merge_to_from)(MessageLite& to, const MessageLite& from_msg),
      internal::MessageCreator message_creator,
      [[maybe_unused]] void (*destroy_message)(MessageLite& msg),  //
      [[maybe_unused]] void (*clear)(MessageLite& msg),
      [[maybe_unused]] size_t (*byte_size_long)(const MessageLite&),
      [[maybe_unused]] uint8_t* (*serialize)(const MessageLite& msg,
                                             uint8_t* ptr,
                                             io::EpsCopyOutputStream* stream),
      uint32_t cached_size_offset,
      std::variant<ReflectionData*, const char*> reflection_or_name)
      : is_initialized(is_initialized),
        merge_to_from(merge_to_from),
        message_creator(message_creator),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        destroy_message(destroy_message),
        clear(clear),
        byte_size_long(byte_size_long),
        serialize(serialize),
#endif  // PROTOBUF_CUSTOM_VTABLE
        cached_size_offset(cached_size_offset),
        is_lite(std::holds_alternative<const char*>(reflection_or_name)),
        aux_data(GetAuxFromVariant(reflection_or_name)) {
  }

  const TcParseTableBase* GetTcParseTable() const;

  const MessageLite* default_instance() const;

  // Defined in message_lite.h.
  MessageLite* New(Arena* arena) const;

  // Defined in message_lite.h.
  MessageLite* PlacementNew(void* mem, Arena* arena) const;

  uint32_t allocation_size() const { return message_creator.allocation_size(); }

  uint8_t alignment() const { return message_creator.alignment(); }

  template <typename MessageLite = google::protobuf::MessageLite>
  PROTOBUF_ALWAYS_INLINE void MergeToFrom(MessageLite& to,
                                          const MessageLite& from) const {
    ABSL_DCHECK(this == to.GetClassData())
        << "Incorrect class data passed to MergeToFrom: expected "
        << to.GetTypeName() << ", got "
        << TypeDependent<MessageLite>(default_instance())->GetTypeName();
    ABSL_DCHECK(this == from.GetClassData())
        << "Invalid call to MergeFrom between types " << to.GetTypeName()
        << " and " << from.GetTypeName();
    this->merge_to_from(to, from);
  }

  std::string DebugName() const;

  // Accessors for reflection related data (!LITE only).
  const Reflection* reflection() const { return reflection_data()->reflection; }
  const Descriptor* descriptor() const { return reflection_data()->descriptor; }

  void set_reflection(const Reflection* reflection) const {
    reflection_data()->reflection = reflection;
  }
  void set_descriptor(const Descriptor* descriptor) const {
    reflection_data()->descriptor = descriptor;
  }

  const internal::DescriptorTable* descriptor_table() const {
    return reflection_data()->descriptor_table;
  }
  const DescriptorMethods* descriptor_methods() const {
    return reflection_data()->descriptor_methods;
  }
  bool has_get_metadata_tracker() const {
    return reflection_data()->get_metadata_tracker != nullptr;
  }
  void get_metadata_tracker() const {
    reflection_data()->get_metadata_tracker();
  }

  ReflectionData* reflection_data() const {
    ABSL_DCHECK(!is_lite);
    return aux_data.reflection_data;
  }

  // Accessors for type name (LITE only).
  const char* type_name() const {
    ABSL_DCHECK(is_lite);
    return aux_data.type_name;
  }

  union ReflectionDataOrTypeName {
    constexpr ReflectionDataOrTypeName(ReflectionData* reflection_data)
        : reflection_data(reflection_data) {}
    constexpr ReflectionDataOrTypeName(const char* type_name)
        : type_name(type_name) {}
    ReflectionData* reflection_data;
    const char* type_name;
  } aux_data;

  static constexpr ReflectionDataOrTypeName GetAuxFromVariant(
      std::variant<ReflectionData*, const char*> reflection_or_name) {
    if (std::holds_alternative<const char*>(reflection_or_name)) {
      return std::get<const char*>(reflection_or_name);
    } else {
      return std::get<ReflectionData*>(reflection_or_name);
    }
  }
};

// We use a secondary vtable for descriptor based methods. This way ClassData
// does not grow with the number of descriptor methods. This avoids extra
// costs in MessageLite.
struct PROTOBUF_EXPORT DescriptorMethods {
  absl::string_view (*get_type_name)(const ClassData* data);
  std::string (*initialization_error_string)(const MessageLite&);
  const internal::TcParseTableBase* (*get_tc_table)(const ClassData*);
  size_t (*space_used_long)(const MessageLite&);
  std::string (*debug_string)(const MessageLite&);
  void (*verify_lazy_field_consistency)(const LazyField&);
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_CLASS_DATA_H__
