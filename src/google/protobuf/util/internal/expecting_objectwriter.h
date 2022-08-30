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

#ifndef GOOGLE_PROTOBUF_UTIL_INTERNAL_EXPECTING_OBJECTWRITER_H__
#define GOOGLE_PROTOBUF_UTIL_INTERNAL_EXPECTING_OBJECTWRITER_H__

// An implementation of ObjectWriter that automatically sets the
// gmock expectations for the response to a method. Every method
// returns the object itself for chaining.
//
// Usage:
//   // Setup
//   MockObjectWriter mock;
//   ExpectingObjectWriter ow(&mock);
//
//   // Set expectation
//   ow.StartObject("")
//       ->RenderString("key", "value")
//     ->EndObject();
//
//   // Actual testing
//   mock.StartObject(absl::string_view())
//         ->RenderString("key", "value")
//       ->EndObject();

#include <cstdint>

#include <google/protobuf/stubs/common.h>
#include <gmock/gmock.h>
#include "absl/strings/string_view.h"
#include <google/protobuf/util/internal/object_writer.h>

namespace google {
namespace protobuf {
namespace util {
namespace converter {

using testing::Eq;
using testing::IsEmpty;
using testing::NanSensitiveDoubleEq;
using testing::NanSensitiveFloatEq;
using testing::Return;
using testing::StrEq;
using testing::TypedEq;

class MockObjectWriter : public ObjectWriter {
 public:
  MockObjectWriter() {}

  MOCK_METHOD(ObjectWriter*, StartObject, (absl::string_view), (override));
  MOCK_METHOD(ObjectWriter*, EndObject, (), (override));
  MOCK_METHOD(ObjectWriter*, StartList, (absl::string_view), (override));
  MOCK_METHOD(ObjectWriter*, EndList, (), (override));
  MOCK_METHOD(ObjectWriter*, RenderBool, (absl::string_view, bool), (override));
  MOCK_METHOD(ObjectWriter*, RenderInt32, (absl::string_view, int32_t),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderUint32, (absl::string_view, uint32_t),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderInt64, (absl::string_view, int64_t),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderUint64, (absl::string_view, uint64_t),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderDouble, (absl::string_view, double),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderFloat, (absl::string_view, float),
              (override));
  MOCK_METHOD(ObjectWriter*, RenderString,
              (absl::string_view, absl::string_view), (override));
  MOCK_METHOD(ObjectWriter*, RenderBytes,
              (absl::string_view, absl::string_view), (override));
  MOCK_METHOD(ObjectWriter*, RenderNull, (absl::string_view), (override));
};

class ExpectingObjectWriter : public ObjectWriter {
 public:
  explicit ExpectingObjectWriter(MockObjectWriter* mock) : mock_(mock) {}
  ExpectingObjectWriter(const ExpectingObjectWriter&) = delete;
  ExpectingObjectWriter& operator=(const ExpectingObjectWriter&) = delete;

  ObjectWriter* StartObject(absl::string_view name) override {
    (name.empty() ? EXPECT_CALL(*mock_, StartObject(IsEmpty()))
                  : EXPECT_CALL(*mock_, StartObject(Eq(std::string(name)))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* EndObject() override {
    EXPECT_CALL(*mock_, EndObject())
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* StartList(absl::string_view name) override {
    (name.empty() ? EXPECT_CALL(*mock_, StartList(IsEmpty()))
                  : EXPECT_CALL(*mock_, StartList(Eq(std::string(name)))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* EndList() override {
    EXPECT_CALL(*mock_, EndList())
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderBool(absl::string_view name, bool value) override {
    (name.empty()
         ? EXPECT_CALL(*mock_, RenderBool(IsEmpty(), TypedEq<bool>(value)))
         : EXPECT_CALL(*mock_,
                       RenderBool(Eq(std::string(name)), TypedEq<bool>(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderInt32(absl::string_view name, int32_t value) override {
    (name.empty()
         ? EXPECT_CALL(*mock_, RenderInt32(IsEmpty(), TypedEq<int32_t>(value)))
         : EXPECT_CALL(*mock_, RenderInt32(Eq(std::string(name)),
                                           TypedEq<int32_t>(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderUint32(absl::string_view name, uint32_t value) override {
    (name.empty() ? EXPECT_CALL(*mock_, RenderUint32(IsEmpty(),
                                                     TypedEq<uint32_t>(value)))
                  : EXPECT_CALL(*mock_, RenderUint32(Eq(std::string(name)),
                                                     TypedEq<uint32_t>(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderInt64(absl::string_view name, int64_t value) override {
    (name.empty()
         ? EXPECT_CALL(*mock_, RenderInt64(IsEmpty(), TypedEq<int64_t>(value)))
         : EXPECT_CALL(*mock_, RenderInt64(Eq(std::string(name)),
                                           TypedEq<int64_t>(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderUint64(absl::string_view name, uint64_t value) override {
    (name.empty() ? EXPECT_CALL(*mock_, RenderUint64(IsEmpty(),
                                                     TypedEq<uint64_t>(value)))
                  : EXPECT_CALL(*mock_, RenderUint64(Eq(std::string(name)),
                                                     TypedEq<uint64_t>(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderDouble(absl::string_view name, double value) override {
    (name.empty()
         ? EXPECT_CALL(*mock_,
                       RenderDouble(IsEmpty(), NanSensitiveDoubleEq(value)))
         : EXPECT_CALL(*mock_, RenderDouble(Eq(std::string(name)),
                                            NanSensitiveDoubleEq(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderFloat(absl::string_view name, float value) override {
    (name.empty()
         ? EXPECT_CALL(*mock_,
                       RenderFloat(IsEmpty(), NanSensitiveFloatEq(value)))
         : EXPECT_CALL(*mock_, RenderFloat(Eq(std::string(name)),
                                           NanSensitiveFloatEq(value))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderString(absl::string_view name,
                             absl::string_view value) override {
    (name.empty() ? EXPECT_CALL(*mock_, RenderString(IsEmpty(),
                                                     TypedEq<absl::string_view>(
                                                         std::string(value))))
                  : EXPECT_CALL(*mock_, RenderString(Eq(std::string(name)),
                                                     TypedEq<absl::string_view>(
                                                         std::string(value)))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }
  ObjectWriter* RenderBytes(absl::string_view name,
                            absl::string_view value) override {
    (name.empty() ? EXPECT_CALL(*mock_, RenderBytes(IsEmpty(),
                                                    TypedEq<absl::string_view>(
                                                        std::string(value))))
                  : EXPECT_CALL(*mock_, RenderBytes(Eq(std::string(name)),
                                                    TypedEq<absl::string_view>(
                                                        std::string(value)))))
        .WillOnce(Return(mock_))
        .RetiresOnSaturation();
    return this;
  }

  ObjectWriter* RenderNull(absl::string_view name) override {
    (name.empty() ? EXPECT_CALL(*mock_, RenderNull(IsEmpty()))
                  : EXPECT_CALL(*mock_, RenderNull(Eq(std::string(name))))
                        .WillOnce(Return(mock_))
                        .RetiresOnSaturation());
    return this;
  }

 private:
  MockObjectWriter* mock_;
};

}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_UTIL_INTERNAL_EXPECTING_OBJECTWRITER_H__
