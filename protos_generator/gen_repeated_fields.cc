// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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
#include "protos_generator/gen_repeated_fields.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "protos_generator/gen_accessors.h"
#include "protos_generator/gen_enums.h"
#include "protos_generator/gen_extensions.h"
#include "protos_generator/gen_utils.h"
#include "protos_generator/names.h"
#include "protos_generator/output.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/names.h"

namespace protos_generator {
namespace protobuf = ::google::protobuf;

// Adds using accessors to reuse base Access class members from a Proxy/CProxy.
void WriteRepeatedFieldUsingAccessors(const protobuf::FieldDescriptor* field,
                                      absl::string_view class_name,
                                      absl::string_view resolved_field_name,
                                      Output& output, bool read_only) {
  if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    output(
        R"cc(
          using $0Access::$1;
          using $0Access::$1_size;
        )cc",
        class_name, resolved_field_name);
    if (!read_only) {
      output(
          R"cc(
            using $0Access::add_$1;
            using $0Access::mutable_$1;
          )cc",
          class_name, resolved_field_name);
    }
  } else {
    output(
        R"cc(
          using $0Access::$1;
          using $0Access::$1_size;
        )cc",
        class_name, resolved_field_name);
    if (!read_only) {
      output(
          R"cc(
            using $0Access::add_$1;
            using $0Access::mutable_$1;
            using $0Access::resize_$1;
            using $0Access::set_$1;
          )cc",
          class_name, resolved_field_name);
    }
  }
}

void WriteRepeatedFieldsInMessageHeader(const protobuf::Descriptor* desc,
                                        const protobuf::FieldDescriptor* field,
                                        absl::string_view resolved_field_name,
                                        absl::string_view resolved_upbc_name,
                                        Output& output) {
  output(
      R"cc(
        inline size_t $1_size() const {
          size_t len;
          $0_$2(msg_, &len);
          return len;
        }
      )cc",
      MessageName(desc), resolved_field_name, resolved_upbc_name);

  if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    output(
        R"cc(
          $1 $2(size_t index) const;
          const ::protos::RepeatedField<const $4>::CProxy $2() const;
          ::protos::Ptr<::protos::RepeatedField<$4>> mutable_$2();
          absl::StatusOr<$0> add_$2();
          $0 mutable_$2(size_t index) const;
        )cc",
        MessagePtrConstType(field, /* const */ false),   // $0
        MessagePtrConstType(field, /* const */ true),    // $1
        resolved_field_name,                             // $2
        resolved_upbc_name,                              // $3
        MessageBaseType(field, /* maybe_const */ false)  // $4
    );
  } else if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
    output(
        R"cc(
          $0 $1(size_t index) const;
          const ::protos::RepeatedField<$0>::CProxy $1() const;
          ::protos::Ptr<::protos::RepeatedField<$0>> mutable_$1();
          bool add_$1($0 val);
          void set_$1(size_t index, $0 val);
          bool resize_$1(size_t len);
        )cc",
        CppConstType(field), resolved_field_name);
  } else {
    output(
        R"cc(
          $0 $1(size_t index) const;
          const ::protos::RepeatedField<$0>::CProxy $1() const;
          ::protos::Ptr<::protos::RepeatedField<$0>> mutable_$1();
          bool add_$1($0 val);
          void set_$1(size_t index, $0 val);
          bool resize_$1(size_t len);
        )cc",
        CppConstType(field), resolved_field_name);
  }
}

void WriteRepeatedMessageAccessor(const protobuf::Descriptor* message,
                                  const protobuf::FieldDescriptor* field,
                                  const absl::string_view resolved_field_name,
                                  const absl::string_view class_name,
                                  Output& output) {
  const char arena_expression[] = "arena_";
  absl::string_view upbc_name = field->name();
  output(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_$5(msg_, &len);
          assert(index < len);
          return ::protos::internal::CreateMessage<$4>(
              (upb_Message*)*(ptr + index), arena_);
        }
      )cc",
      class_name, MessagePtrConstType(field, /* is_const */ true),
      resolved_field_name, MessageName(message),
      MessageBaseType(field, /* maybe_const */ false), upbc_name);
  output(
      R"cc(
        absl::StatusOr<$1> $0::add_$2() {
          auto new_msg = $3_add_$6(msg_, $5);
          if (!new_msg) {
            return ::protos::MessageAllocationError();
          }
          return ::protos::internal::CreateMessageProxy<$4>((upb_Message*)new_msg, $5);
        }
      )cc",
      class_name, MessagePtrConstType(field, /* const */ false),
      resolved_field_name, MessageName(message),
      MessageBaseType(field, /* maybe_const */ false), arena_expression,
      upbc_name);
  output(
      R"cc(
        $1 $0::mutable_$2(size_t index) const {
          size_t len;
          auto* ptr = $3_$6(msg_, &len);
          assert(index < len);
          return ::protos::internal::CreateMessageProxy<$4>(
              (upb_Message*)*(ptr + index), $5);
        }
      )cc",
      class_name, MessagePtrConstType(field, /* is_const */ false),
      resolved_field_name, MessageName(message),
      MessageBaseType(field, /* maybe_const */ false), arena_expression,
      upbc_name);
  output(
      R"cc(
        const ::protos::RepeatedField<const $1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::protos::RepeatedField<const $1>::CProxy(arr, arena_);
        };
        ::protos::Ptr<::protos::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::protos::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      MessageBaseType(field, /* maybe_const */ false),         // $1
      resolved_field_name,                                     // $2
      MessageName(message),                                    // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

void WriteRepeatedStringAccessor(const protobuf::Descriptor* message,
                                 const protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Output& output) {
  absl::string_view upbc_name = field->name();
  output(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          return ::protos::UpbStrToStringView(*(ptr + index));
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        bool $0::resize_$1(size_t len) {
          return $2_resize_$3(msg_, len, arena_);
        }
      )cc",
      class_name, resolved_field_name, MessageName(message), upbc_name);
  output(
      R"cc(
        bool $0::add_$2($1 val) {
          return $3_add_$4(msg_, ::protos::UpbStrFromStringView(val, arena_), arena_);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        void $0::set_$2(size_t index, $1 val) {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          *(ptr + index) = ::protos::UpbStrFromStringView(val, arena_);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        const ::protos::RepeatedField<$1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::protos::RepeatedField<$1>::CProxy(arr, arena_);
        };
        ::protos::Ptr<::protos::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::protos::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      CppConstType(field),                                     // $1
      resolved_field_name,                                     // $2
      MessageName(message),                                    // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

void WriteRepeatedScalarAccessor(const protobuf::Descriptor* message,
                                 const protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Output& output) {
  absl::string_view upbc_name = field->name();
  output(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          return *(ptr + index);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        bool $0::resize_$1(size_t len) {
          return $2_resize_$3(msg_, len, arena_);
        }
      )cc",
      class_name, resolved_field_name, MessageName(message), upbc_name);
  output(
      R"cc(
        bool $0::add_$2($1 val) { return $3_add_$4(msg_, val, arena_); }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        void $0::set_$2(size_t index, $1 val) {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          *(ptr + index) = val;
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      MessageName(message), upbc_name);
  output(
      R"cc(
        const ::protos::RepeatedField<$1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::protos::RepeatedField<$1>::CProxy(arr, arena_);
        };
        ::protos::Ptr<::protos::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::protos::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      CppConstType(field),                                     // $1
      resolved_field_name,                                     // $2
      MessageName(message),                                    // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

}  // namespace protos_generator
