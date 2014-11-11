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

// This header defines the RepeatedFieldRef class template used to access
// repeated fields with protobuf reflection API.
#ifndef GOOGLE_PROTOBUF_REFLECTION_H__
#define GOOGLE_PROTOBUF_REFLECTION_H__

#include <google/protobuf/message.h>

namespace google {
namespace protobuf {
namespace internal {
template<typename T, typename Enable = void>
struct RefTypeTraits;
}  // namespace internal

template<typename T>
RepeatedFieldRef<T> Reflection::GetRepeatedFieldRef(
    const Message& message, const FieldDescriptor* field) const {
  return RepeatedFieldRef<T>(message, field);
}

template<typename T>
MutableRepeatedFieldRef<T> Reflection::GetMutableRepeatedFieldRef(
    Message* message, const FieldDescriptor* field) const {
  return MutableRepeatedFieldRef<T>(message, field);
}

// RepeatedFieldRef definition for non-message types.
template<typename T>
class RepeatedFieldRef<
    T, typename internal::enable_if<!internal::is_base_of<Message, T>::value>::type> {
  typedef typename internal::RefTypeTraits<T>::iterator IteratorType;
  typedef typename internal::RefTypeTraits<T>::AccessorType AccessorType;

 public:
  bool empty() const {
    return accessor_->IsEmpty(data_);
  }
  int size() const {
    return accessor_->Size(data_);
  }
  T Get(int index) const {
    return accessor_->template Get<T>(data_, index);
  }

  typedef IteratorType iterator;
  typedef IteratorType const_iterator;
  iterator begin() const {
    return iterator(data_, accessor_, true);
  }
  iterator end() const {
    return iterator(data_, accessor_, false);
  }

 private:
  friend class Reflection;
  RepeatedFieldRef(
      const Message& message,
      const FieldDescriptor* field) {
    const Reflection* reflection = message.GetReflection();
    data_ = reflection->RepeatedFieldData(
        const_cast<Message*>(&message), field,
        internal::RefTypeTraits<T>::cpp_type, NULL);
    accessor_ = reflection->RepeatedFieldAccessor(field);
  }

  const void* data_;
  const AccessorType* accessor_;
};

// MutableRepeatedFieldRef definition for non-message types.
template<typename T>
class MutableRepeatedFieldRef<
    T, typename internal::enable_if<!internal::is_base_of<Message, T>::value>::type> {
  typedef typename internal::RefTypeTraits<T>::AccessorType AccessorType;

 public:
  bool empty() const {
    return accessor_->IsEmpty(data_);
  }
  int size() const {
    return accessor_->Size(data_);
  }
  T Get(int index) const {
    return accessor_->template Get<T>(data_, index);
  }

  void Set(int index, const T& value) const {
    accessor_->template Set<T>(data_, index, value);
  }
  void Add(const T& value) const {
    accessor_->template Add<T>(data_, value);
  }
  void RemoveLast() const {
    accessor_->RemoveLast(data_);
  }
  void SwapElements(int index1, int index2) const {
    accessor_->SwapElements(data_, index1, index2);
  }
  void Clear() const {
    accessor_->Clear(data_);
  }

  void Swap(const MutableRepeatedFieldRef& other) const {
    accessor_->Swap(data_, other.accessor_, other.data_);
  }

  template<typename Container>
  void MergeFrom(const Container& container) const {
    typedef typename Container::const_iterator Iterator;
    for (Iterator it = container.begin(); it != container.end(); ++it) {
      Add(*it);
    }
  }
  template<typename Container>
  void CopyFrom(const Container& container) const {
    Clear();
    MergeFrom(container);
  }

 private:
  friend class Reflection;
  MutableRepeatedFieldRef(
      Message* message,
      const FieldDescriptor* field) {
    const Reflection* reflection = message->GetReflection();
    data_ = reflection->RepeatedFieldData(
        message, field, internal::RefTypeTraits<T>::cpp_type, NULL);
    accessor_ = reflection->RepeatedFieldAccessor(field);
  }

  void* data_;
  const AccessorType* accessor_;
};

// RepeatedFieldRef definition for message types.
template<typename T>
class RepeatedFieldRef<
    T, typename internal::enable_if<internal::is_base_of<Message, T>::value>::type> {
  typedef typename internal::RefTypeTraits<T>::iterator IteratorType;
  typedef typename internal::RefTypeTraits<T>::AccessorType AccessorType;

 public:
  bool empty() const {
    return accessor_->IsEmpty(data_);
  }
  int size() const {
    return accessor_->Size(data_);
  }
  // This method returns a reference to the underlying message object if it
  // exists. If a message object doesn't exist (e.g., data stored in serialized
  // form), scratch_space will be filled with the data and a reference to it
  // will be returned.
  //
  // Example:
  //   RepeatedFieldRef<Message> h = ...
  //   unique_ptr<Message> scratch_space(h.NewMessage());
  //   const Message& item = h.Get(index, scratch_space.get());
  const T& Get(int index, T* scratch_space) const {
    return *static_cast<const T*>(accessor_->Get(data_, index, scratch_space));
  }
  // Create a new message of the same type as the messages stored in this
  // repeated field. Caller takes ownership of the returned object.
  T* NewMessage() const {
    return static_cast<T*>(default_instance_->New());
  }

  typedef IteratorType iterator;
  typedef IteratorType const_iterator;
  iterator begin() const {
    return iterator(data_, accessor_, true, NewMessage());
  }
  iterator end() const {
    return iterator(data_, accessor_, false, NewMessage());
  }

 private:
  friend class Reflection;
  RepeatedFieldRef(
      const Message& message,
      const FieldDescriptor* field) {
    const Reflection* reflection = message.GetReflection();
    data_ = reflection->RepeatedFieldData(
        const_cast<Message*>(&message), field,
        internal::RefTypeTraits<T>::cpp_type,
        internal::RefTypeTraits<T>::GetMessageFieldDescriptor());
    accessor_ = reflection->RepeatedFieldAccessor(field);
    default_instance_ =
        reflection->GetMessageFactory()->GetPrototype(field->message_type());
  }

  const void* data_;
  const AccessorType* accessor_;
  const Message* default_instance_;
};

// MutableRepeatedFieldRef definition for non-message types.
template<typename T>
class MutableRepeatedFieldRef<
    T, typename internal::enable_if<internal::is_base_of<Message, T>::value>::type> {
  typedef typename internal::RefTypeTraits<T>::AccessorType AccessorType;

 public:
  bool empty() const {
    return accessor_->IsEmpty(data_);
  }
  int size() const {
    return accessor_->Size(data_);
  }
  // See comments for RepeatedFieldRef<Message>::Get()
  const T& Get(int index, T* scratch_space) const {
    return *static_cast<const T*>(accessor_->Get(data_, index, scratch_space));
  }
  // Create a new message of the same type as the messages stored in this
  // repeated field. Caller takes ownership of the returned object.
  T* NewMessage() const {
    return static_cast<T*>(default_instance_->New());
  }

  void Set(int index, const T& value) const {
    accessor_->Set(data_, index, &value);
  }
  void Add(const T& value) const {
    accessor_->Add(data_, &value);
  }
  void RemoveLast() const {
    accessor_->RemoveLast(data_);
  }
  void SwapElements(int index1, int index2) const {
    accessor_->SwapElements(data_, index1, index2);
  }
  void Clear() const {
    accessor_->Clear(data_);
  }

  void Swap(const MutableRepeatedFieldRef& other) const {
    accessor_->Swap(data_, other.accessor_, other.data_);
  }

  template<typename Container>
  void MergeFrom(const Container& container) const {
    typedef typename Container::const_iterator Iterator;
    for (Iterator it = container.begin(); it != container.end(); ++it) {
      Add(*it);
    }
  }
  template<typename Container>
  void CopyFrom(const Container& container) const {
    Clear();
    MergeFrom(container);
  }

 private:
  friend class Reflection;
  MutableRepeatedFieldRef(
      Message* message,
      const FieldDescriptor* field) {
    const Reflection* reflection = message->GetReflection();
    data_ = reflection->RepeatedFieldData(
        message, field, internal::RefTypeTraits<T>::cpp_type,
        internal::RefTypeTraits<T>::GetMessageFieldDescriptor());
    accessor_ = reflection->RepeatedFieldAccessor(field);
    default_instance_ =
        reflection->GetMessageFactory()->GetPrototype(field->message_type());
  }

  void* data_;
  const AccessorType* accessor_;
  const Message* default_instance_;
};
}  // namespace protobuf
}  // namespace google

// Implementation details for (Mutable)RepeatedFieldRef.
#include <google/protobuf/repeated_field_reflection.h>

#endif  // GOOGLE_PROTOBUF_REFLECTION_H__
