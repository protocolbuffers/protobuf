// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_IMPLICIT_WEAK_MESSAGE_H__
#define GOOGLE_PROTOBUF_IMPLICIT_WEAK_MESSAGE_H__

#include <string>

#include "google/protobuf/arena.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

// Must be included last.
#include "google/protobuf/port_def.inc"

// This file is logically internal-only and should only be used by protobuf
// generated code.

namespace google {
namespace protobuf {
namespace internal {

// An implementation of MessageLite that treats all data as unknown. This type
// acts as a placeholder for an implicit weak field in the case where the true
// message type does not get linked into the binary.
class PROTOBUF_EXPORT ImplicitWeakMessage : public MessageLite {
 public:
  ImplicitWeakMessage() : ImplicitWeakMessage(nullptr) {}
  explicit constexpr ImplicitWeakMessage(ConstantInitialized)
      : data_(nullptr) {}
  ImplicitWeakMessage(const ImplicitWeakMessage&) = delete;
  ImplicitWeakMessage& operator=(const ImplicitWeakMessage&) = delete;

  // Arena enabled constructors: for internal use only.
  ImplicitWeakMessage(internal::InternalVisibility, Arena* arena)
      : ImplicitWeakMessage(arena) {}

  // TODO: make this constructor private
  explicit ImplicitWeakMessage(Arena* arena)
      : MessageLite(arena), data_(new std::string) {}

  ~ImplicitWeakMessage() override {
    // data_ will be null in the default instance, but we can safely call delete
    // here because the default instance will never be destroyed.
    delete data_;
  }

  static const ImplicitWeakMessage* default_instance();

  const ClassData* GetClassData() const final {
    struct Data {
      ClassData header;
      char name[1];
    };
    static constexpr Data data = {{}, ""};
    return &data.header;
  }

  MessageLite* New(Arena* arena) const override {
    return Arena::CreateMessage<ImplicitWeakMessage>(arena);
  }

  void Clear() override { data_->clear(); }

  bool IsInitialized() const override { return true; }

  void CheckTypeAndMergeFrom(const MessageLite& other) override {
    const std::string* other_data =
        static_cast<const ImplicitWeakMessage&>(other).data_;
    if (other_data != nullptr) {
      data_->append(*other_data);
    }
  }

  const char* _InternalParse(const char* ptr, ParseContext* ctx) final;

  size_t ByteSizeLong() const override {
    return data_ == nullptr ? 0 : data_->size();
  }

  uint8_t* _InternalSerialize(uint8_t* target,
                              io::EpsCopyOutputStream* stream) const final {
    if (data_ == nullptr) {
      return target;
    }
    return stream->WriteRaw(data_->data(), static_cast<int>(data_->size()),
                            target);
  }

  typedef void InternalArenaConstructable_;

 private:
  // This std::string is allocated on the heap, but we use a raw pointer so that
  // the default instance can be constant-initialized. In the const methods, we
  // have to handle the possibility of data_ being null.
  std::string* data_;
};

struct ImplicitWeakMessageDefaultType;
extern ImplicitWeakMessageDefaultType implicit_weak_message_default_instance;

}  // namespace internal

template <typename T>
struct WeakRepeatedPtrField {
  using Base = RepeatedPtrField<MessageLite>;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  constexpr WeakRepeatedPtrField() : weak() {}
  WeakRepeatedPtrField(const WeakRepeatedPtrField& rhs)
      : WeakRepeatedPtrField(nullptr, rhs) {}

  // Arena enabled constructors: for internal use only.
  WeakRepeatedPtrField(internal::InternalVisibility, Arena* arena)
      : WeakRepeatedPtrField(arena) {}
  WeakRepeatedPtrField(internal::InternalVisibility, Arena* arena,
                       const WeakRepeatedPtrField& rhs)
      : WeakRepeatedPtrField(arena, rhs) {}

  // TODO: make this constructor private
  explicit WeakRepeatedPtrField(Arena* arena) : weak(arena) {}

  using iterator = Base::iterator;
  using const_iterator = Base::const_iterator;
  using pointer_iterator = Base::pointer_iterator;
  using const_pointer_iterator = Base::const_pointer_iterator;

  bool empty() const { return weak.empty(); }
  iterator begin() { return weak.begin(); }
  const_iterator begin() const { return weak.begin(); }
  const_iterator cbegin() const { return begin(); }
  iterator end() { return weak.end(); }
  const_iterator end() const { return weak.end(); }
  const_iterator cend() const { return end(); }
  pointer_iterator pointer_begin() { return weak.pointer_begin(); }
  const_pointer_iterator pointer_begin() const { return weak.pointer_begin(); }
  pointer_iterator pointer_end() { return weak.pointer_end(); }
  const_pointer_iterator pointer_end() const { return weak.pointer_end(); }

  T* Add() { return static_cast<T*>(weak.Add()); }
  void Clear() { weak.Clear(); }
  void MergeFrom(const WeakRepeatedPtrField& other) {
    weak.MergeFrom(other.weak);
  }
  void InternalSwap(WeakRepeatedPtrField* PROTOBUF_RESTRICT other) {
    weak.InternalSwap(other->weak);
  }

  RepeatedPtrField<MessageLite> weak;

 private:
  WeakRepeatedPtrField(Arena* arena, const WeakRepeatedPtrField& rhs)
      : WeakRepeatedPtrField(arena) {
    MergeFrom(rhs);
  }
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_IMPLICIT_WEAK_MESSAGE_H__
