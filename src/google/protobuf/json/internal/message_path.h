// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__
#define GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
// A path in a Protobuf message, annotated specifically for producing nice
// errors.
class MessagePath {
  class Popper;

 public:
  explicit MessagePath(absl::string_view message_root)
      : components_(
            {Component{FieldDescriptor::TYPE_MESSAGE, message_root, "", -1}}) {}

  // Pushes a new field component onto the path, returning an RAII object that
  // pops it on scope exit.
  //
  // `f` is invoked synchronously with a setter callback taking
  // `(absl::string_view field_name, FieldDescriptor::Type type,
  //  absl::string_view type_name = "")`, where `type_name` is an optional name
  // for message or enum fields. `f` must call that setter once:
  //
  //   auto pop = path.Push([field](auto set) {
  //     set(field->name(), field->type(), ...);
  //   });
  //
  // Accepting a lambda lets `PushImpl` keep both vector growth and argument
  // evaluation (e.g. out-of-line descriptor lookups) out of the caller's
  // recursive stack frame, avoiding call-crossing spills in recursive callers.
  template <typename F>
  Popper Push(F f) {
    PushImpl(f);
    return Popper(this);
  }

  // Increments the index of this field, indicating it is a repeated field.
  //
  // The first time this is called, the field will be marked as repeated and
  // the index will become 0.
  void NextRepeated() { ++components_.back().repeated_index; }

  // Appends a description of the current state of the path to `out`.
  void Describe(std::string& out) const;

 private:
  class [[nodiscard]] Popper {
   public:
    explicit Popper(MessagePath* path) : path_(path) {}
    ~Popper() { path_->components_.pop_back(); }

    // No copy/move/assign to make the type cheaper.
    Popper(const Popper&) = delete;
    Popper& operator=(const Popper&) = delete;

   private:
    MessagePath* path_;
  };

  struct Component {
    FieldDescriptor::Type type;
    absl::string_view type_name, field_name;
    int32_t repeated_index;
  };

  // Out of line on purpose to prevent vector growth and `f` from growing the
  // stack of the caller.
  template <typename F>
  PROTOBUF_NOINLINE void PushImpl(F f) {
    auto& cmp = components_.emplace_back();
    f([&cmp](absl::string_view field_name, FieldDescriptor::Type type,
             absl::string_view type_name = "") {
      // -1 makes it so the first call to NextRepeated makes the index 0.
      cmp = Component{type, type_name, field_name, -1};
    });
  }

  std::vector<Component> components_;
};
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__
