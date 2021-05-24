// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_FIELD_ACCESS_LISTENER_H__
#define GOOGLE_PROTOBUF_FIELD_ACCESS_LISTENER_H__

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/map.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/repeated_field.h>


namespace google {
namespace protobuf {
namespace internal {
template <typename T>
struct ResolvedType {
  using type = T;
};
}  // namespace internal
// Tracks the events of field accesses for all protos
// that are built with --inject_field_listener_events. This is a global
// interface which you must implement yourself and register with
// RegisterListener() function. All events consist of Descriptors,
// FieldAccessTypes and the underlying storage for tracking the memory which is
// accessed where possible and makes sense. Users are responsible for the
// implementations to be thread safe.
class FieldAccessListener {
 public:
  FieldAccessListener() = default;
  virtual ~FieldAccessListener() = default;

  // The memory annotations of the proto fields that are touched by the
  // accessors. They are returned as if the operation completes.
  struct DataAnnotation {
    DataAnnotation() = default;
    DataAnnotation(const void* other_address, size_t other_size)
        : address(other_address), size(other_size) {}
    const void* address = nullptr;
    size_t size = 0;
  };
  using AddressInfo = std::vector<DataAnnotation>;
  using AddressInfoExtractor = std::function<AddressInfo()>;

  enum class FieldAccessType {
    kAdd,          // add_<field>(f)
    kAddMutable,   // add_<field>()
    kGet,          // <field>() and <repeated_field>(i)
    kClear,        // clear_<field>()
    kHas,          // has_<field>()
    kList,         // <repeated_field>()
    kMutable,      // mutable_<field>()
    kMutableList,  // mutable_<repeated_field>()
    kRelease,      // release_<field>()
    kSet,          // set_<field>() and set_<repeated_field>(i)
    kSize,         // <repeated_field>_size()
  };

  static FieldAccessListener* GetListener();

  // Registers the field listener, can be called only once, |listener| must
  // outlive all proto accesses (in most cases, the lifetime of the program).
  static void RegisterListener(FieldAccessListener* listener);

  // All field accessors noted in FieldAccessType have this call.
  // |extractor| extracts the address info from the field
  virtual void OnFieldAccess(const AddressInfoExtractor& extractor,
                             const FieldDescriptor* descriptor,
                             FieldAccessType access_type) = 0;

  // Side effect calls.
  virtual void OnDeserializationAccess(const Message* message) = 0;
  virtual void OnSerializationAccess(const Message* message) = 0;
  virtual void OnReflectionAccess(const Descriptor* descriptor) = 0;
  virtual void OnByteSizeAccess(const Message* message) = 0;
  // We can probably add more if we need to, like {Merge,Copy}{From}Access.

  // Extracts all the addresses from the underlying fields.
  template <typename T>
  AddressInfo ExtractFieldInfo(const T* field_value);


 private:
  template <typename T>
  AddressInfo ExtractFieldInfoSpecific(const T* field_value,
                                       internal::ResolvedType<T>);

  AddressInfo ExtractFieldInfoSpecific(const Message* field_value,
                                       internal::ResolvedType<Message>);

  AddressInfo ExtractFieldInfoSpecific(const std::string* field_value,
                                       internal::ResolvedType<std::string>);

  AddressInfo ExtractFieldInfoSpecific(
      const internal::ArenaStringPtr* field_value,
      internal::ResolvedType<internal::ArenaStringPtr>);

  template <typename T>
  AddressInfo ExtractFieldInfoSpecific(
      const RepeatedField<T>* field_value,
      internal::ResolvedType<RepeatedField<T>>);

  template <typename T>
  AddressInfo ExtractFieldInfoSpecific(
      const RepeatedPtrField<T>* field_value,
      internal::ResolvedType<RepeatedPtrField<T>>);

  template <typename K, typename V>
  AddressInfo ExtractFieldInfoSpecific(const Map<K, V>* field_value,
                                       internal::ResolvedType<Map<K, V>>);

  static internal::once_flag register_once_;
  static FieldAccessListener* field_listener_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldAccessListener);
};

template <typename T>
inline FieldAccessListener::AddressInfo FieldAccessListener::ExtractFieldInfo(
    const T* field_value) {
  return ExtractFieldInfoSpecific(field_value, internal::ResolvedType<T>());
}


template <typename T>
inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(const T* field_value,
                                              internal::ResolvedType<T>) {
  static_assert(std::is_trivial<T>::value,
                "This overload should be chosen only for trivial types");
  return FieldAccessListener::AddressInfo{FieldAccessListener::DataAnnotation(
      static_cast<const void*>(field_value), sizeof(*field_value))};
}

inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(
    const std::string* field_value, internal::ResolvedType<std::string>) {
  return FieldAccessListener::AddressInfo{FieldAccessListener::DataAnnotation(
      static_cast<const void*>(field_value->c_str()), field_value->length())};
}

inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(
    const internal::ArenaStringPtr* field_value,
    internal::ResolvedType<internal::ArenaStringPtr>) {
  return FieldAccessListener::ExtractFieldInfoSpecific(
      field_value->GetPointer(), internal::ResolvedType<std::string>());
}

template <typename T>
inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(
    const RepeatedField<T>* field_value,
    internal::ResolvedType<RepeatedField<T>>) {
  // TODO(jianzhouzh): This can cause data races. Synchronize this if needed.
  FieldAccessListener::AddressInfo address_info;
  address_info.reserve(field_value->size());
  for (int i = 0, ie = field_value->size(); i < ie; ++i) {
    auto sub = ExtractFieldInfoSpecific(&field_value->Get(i),
                                        internal::ResolvedType<T>());
    address_info.insert(address_info.end(), sub.begin(), sub.end());
  }
  return address_info;
}

template <typename T>
inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(
    const RepeatedPtrField<T>* field_value,
    internal::ResolvedType<RepeatedPtrField<T>>) {
  FieldAccessListener::AddressInfo address_info;
  // TODO(jianzhouzh): This can cause data races. Synchronize this if needed.
  address_info.reserve(field_value->size());
  for (int i = 0, ie = field_value->size(); i < ie; ++i) {
    auto sub = ExtractFieldInfoSpecific(&field_value->Get(i),
                                        internal::ResolvedType<T>());
    address_info.insert(address_info.end(), sub.begin(), sub.end());
  }
  return address_info;
}

template <typename K, typename V>
inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(
    const Map<K, V>* field_value, internal::ResolvedType<Map<K, V>>) {
  // TODO(jianzhouzh): This can cause data races. Synchronize this if needed.
  FieldAccessListener::AddressInfo address_info;
  address_info.reserve(field_value->size());
  for (auto it = field_value->begin(); it != field_value->end(); ++it) {
    auto sub_first =
        ExtractFieldInfoSpecific(&it->first, internal::ResolvedType<K>());
    auto sub_second =
        ExtractFieldInfoSpecific(&it->second, internal::ResolvedType<V>());
    address_info.insert(address_info.end(), sub_first.begin(), sub_first.end());
    address_info.insert(address_info.end(), sub_second.begin(),
                        sub_second.end());
  }
  return address_info;
}

inline FieldAccessListener::AddressInfo
FieldAccessListener::ExtractFieldInfoSpecific(const Message* field_value,
                                              internal::ResolvedType<Message>) {
  // TODO(jianzhouzh): implement and adjust all annotations in the compiler.
  return {};
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_FIELD_ACCESS_LISTENER_H__
