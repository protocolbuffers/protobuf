// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/expected.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb/error.h"
#include "hpb/internal/error_access.h"
#include "hpb/ptr.h"

namespace hpb {
namespace {

using ::hpb_unittest::protos::TestModel;

TEST(ErrorTest, MemoryLayoutAndTriviality) {
  static_assert(sizeof(Error) == 8,
                "Error must fit in a single 64-bit register for zero-cost "
                "return");
  static_assert(std::is_trivially_copyable_v<Error>,
                "Error must be trivially copyable");
  static_assert(std::is_trivially_destructible_v<Error>,
                "Error must be trivially destructible");
}

TEST(ErrorTest, DefaultConstructor) {
  Error err;
  EXPECT_EQ(err.code(), ErrorCode::kUnknown);
  EXPECT_EQ(err.message(), "Unknown error");
}

TEST(ErrorTest, FactoryMethodsAndMessage) {
  Error oom = internal::ErrorAccess::OutOfMemory();
  EXPECT_EQ(oom.code(), ErrorCode::kOutOfMemory);
  EXPECT_EQ(oom.extension_number(), 0);
  EXPECT_EQ(oom.decode_status(), DecodeStatus::kOk);
  EXPECT_EQ(oom.encode_status(), EncodeStatus::kOk);
  EXPECT_EQ(oom.message(), "Out of memory");

  Error not_found = internal::ErrorAccess::ExtensionNotFound(12345);
  EXPECT_EQ(not_found.code(), ErrorCode::kExtensionNotFound);
  EXPECT_EQ(not_found.extension_number(), 12345);
  EXPECT_EQ(not_found.decode_status(), DecodeStatus::kOk);
  EXPECT_EQ(not_found.encode_status(), EncodeStatus::kOk);
  EXPECT_EQ(not_found.message(), "Extension not found");

  Error decode = internal::ErrorAccess::DecodeError(DecodeStatus::kMalformed);
  EXPECT_EQ(decode.code(), ErrorCode::kDecodeError);
  EXPECT_EQ(decode.extension_number(), 0);
  EXPECT_EQ(decode.decode_status(), DecodeStatus::kMalformed);
  EXPECT_EQ(decode.encode_status(), EncodeStatus::kOk);
  EXPECT_EQ(decode.message(), "Malformed wire format");

  Error encode =
      internal::ErrorAccess::EncodeError(EncodeStatus::kMaxSizeExceeded);
  EXPECT_EQ(encode.code(), ErrorCode::kEncodeError);
  EXPECT_EQ(encode.extension_number(), 0);
  EXPECT_EQ(encode.decode_status(), DecodeStatus::kOk);
  EXPECT_EQ(encode.encode_status(), EncodeStatus::kMaxSizeExceeded);
  EXPECT_EQ(encode.message(), "Message size exceeds 2GB limit");
}

TEST(ErrorTest, StatusAndImplicitConversion) {
  Error oom = internal::ErrorAccess::OutOfMemory();
  absl::Status absl_oom = oom;
  EXPECT_EQ(absl_oom.code(), absl::StatusCode::kResourceExhausted);
  EXPECT_EQ(absl_oom.message(), "Out of memory");

  Error not_found = internal::ErrorAccess::ExtensionNotFound(42);
  absl::Status absl_not_found = not_found;
  EXPECT_EQ(absl_not_found.code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(absl_not_found.message(), "Extension 42 not found");

  Error decode =
      internal::ErrorAccess::DecodeError(DecodeStatus::kMaxDepthExceeded);
  absl::Status absl_decode = decode;
  EXPECT_EQ(absl_decode.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(absl_decode.message(), "Decode error: Recursion depth exceeded");

  Error encode =
      internal::ErrorAccess::EncodeError(EncodeStatus::kMaxSizeExceeded);
  absl::Status absl_encode = encode;
  EXPECT_EQ(absl_encode.code(), absl::StatusCode::kInternal);
  EXPECT_EQ(absl_encode.message(),
            "Encode error: Message size exceeds 2GB limit");
}

TEST(ErrorTest, EqualityAndStringification) {
  Error e1 = internal::ErrorAccess::ExtensionNotFound(100);
  Error e2 = internal::ErrorAccess::ExtensionNotFound(100);
  Error e3 = internal::ErrorAccess::ExtensionNotFound(200);
  EXPECT_EQ(e1, e2);
  EXPECT_NE(e1, e3);

  EXPECT_EQ(absl::StrCat(e1), "Extension 100 not found");

  Error oom = internal::ErrorAccess::OutOfMemory();
  EXPECT_EQ(absl::StrCat(oom), "Out of memory");

  Error decode = internal::ErrorAccess::DecodeError(DecodeStatus::kMalformed);
  EXPECT_EQ(absl::StrCat(decode), "Decode error: Malformed wire format");

  Error encode =
      internal::ErrorAccess::EncodeError(EncodeStatus::kMaxSizeExceeded);
  EXPECT_EQ(absl::StrCat(encode),
            "Encode error: Message size exceeds 2GB limit");
}

TEST(ExpectedVoidTest, SuccessState) {
  Expected<void> res;
  EXPECT_TRUE(res.has_value());
  EXPECT_TRUE(res.ok());
  EXPECT_TRUE(res.status().ok());

  absl::Status absl_s = res;
  EXPECT_TRUE(absl_s.ok());
}

TEST(ExpectedVoidTest, ErrorState) {
  Expected<void> res = internal::ErrorAccess::ExtensionNotFound(777);
  EXPECT_FALSE(res.has_value());
  EXPECT_FALSE(res.ok());
  EXPECT_EQ(res.error().code(), ErrorCode::kExtensionNotFound);

  absl::Status absl_s = res;
  EXPECT_FALSE(absl_s.ok());
  EXPECT_EQ(absl_s.code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(absl_s.message(), "Extension 777 not found");
}

TEST(ExpectedVoidTest, ReturnIfErrorHandling) {
  const auto helper = [](bool succeed) -> absl::Status {
    const auto do_work = [succeed]() -> Expected<void> {
      if (succeed) {
        return {};
      }
      return internal::ErrorAccess::OutOfMemory();
    };

    const Expected<void> res = do_work();
    if (!res.ok()) {
      return res;
    }
    return absl::OkStatus();
  };

  EXPECT_TRUE(helper(true).ok());
  absl::Status err = helper(false);
  EXPECT_FALSE(err.ok());
  EXPECT_EQ(err.code(), absl::StatusCode::kResourceExhausted);
}

TEST(ExpectedTest, ValueState) {
  Expected<int> res(42);
  EXPECT_TRUE(res.has_value());
  EXPECT_TRUE(res.ok());
  EXPECT_EQ(res.value(), 42);
  EXPECT_EQ(*res, 42);
  EXPECT_EQ(res.value_or(0), 42);
  EXPECT_TRUE(res.status().ok());
}

TEST(ExpectedTest, ErrorState) {
  Expected<int> res = internal::ErrorAccess::ExtensionNotFound(99);
  EXPECT_FALSE(res.has_value());
  EXPECT_FALSE(res.ok());
  EXPECT_EQ(res.error().code(), ErrorCode::kExtensionNotFound);
  EXPECT_EQ(res.value_or(123), 123);

  absl::Status s = res.status();
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(s.code(), absl::StatusCode::kNotFound);
}

TEST(ExpectedTest, ImplicitConversionToAbslStatusOr) {
  const auto returns_absl = []() -> absl::StatusOr<int> {
    Expected<int> hpb_val(50);
    return hpb_val;
  };
  absl::StatusOr<int> res = returns_absl();
  ASSERT_TRUE(res.ok());
  EXPECT_EQ(*res, 50);

  const auto returns_absl_error = []() -> absl::StatusOr<int> {
    Expected<int> hpb_err = internal::ErrorAccess::ExtensionNotFound(99);
    return hpb_err;
  };
  absl::StatusOr<int> err_res = returns_absl_error();
  ASSERT_FALSE(err_res.ok());
  EXPECT_EQ(err_res.status().code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(err_res.status().message(), "Extension 99 not found");
}

TEST(ExpectedTest, StatusOrPropagation) {
  const auto helper = [](bool succeed) -> absl::StatusOr<std::string> {
    const auto get_val = [succeed]() -> Expected<std::string> {
      if (succeed) {
        return std::string("success");
      }
      return internal::ErrorAccess::ExtensionNotFound(100);
    };

    Expected<std::string> res = get_val();
    if (!res.ok()) {
      return res;
    }
    return *res + " processed";
  };

  auto ok_res = helper(true);
  ASSERT_TRUE(ok_res.ok());
  EXPECT_EQ(*ok_res, "success processed");

  auto err_res = helper(false);
  ASSERT_FALSE(err_res.ok());
  EXPECT_EQ(err_res.status().code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(err_res.status().message(), "Extension 100 not found");
}

TEST(ExpectedTest, MoveOnlyType) {
  auto ptr = std::make_unique<int>(123);
  Expected<std::unique_ptr<int>> res(std::move(ptr));
  EXPECT_TRUE(res.ok());
  EXPECT_EQ(**res, 123);

  std::unique_ptr<int> moved = *std::move(res);
  EXPECT_EQ(*moved, 123);
}

TEST(ExpectedTest, PtrErgonomics) {
  TestModel model;
  model.set_optional_int64(42);
  hpb::Ptr<TestModel> ptr(&model);

  Expected<hpb::Ptr<TestModel>> res(ptr);
  EXPECT_TRUE(res.ok());

  // Standard operator-> returns pointer to Ptr<TestModel>:
  EXPECT_EQ((*res)->optional_int64(), 42);
  EXPECT_EQ(res.value()->optional_int64(), 42);
}

TEST(ExpectedTest, SizeRequirement) {
  // Raw pointer is 8B, Error is 8B -> Expected<int*> is 16B.
  EXPECT_LE(sizeof(Expected<int*>), 16);
  // Ptr<T> is 16B in upb, Error is 8B -> Expected<Ptr<T>> is 24B.
  EXPECT_LE(sizeof(Expected<hpb::Ptr<TestModel>>), 24);
  // void Expected is at most 16B.
  EXPECT_LE(sizeof(Expected<void>), 16);
}

#ifndef NDEBUG
TEST(ExpectedTest, AbortsOnInvalidAccess) {
  Expected<int> err(internal::ErrorAccess::OutOfMemory());
  EXPECT_DEATH(err.value(), "Attempting to access value");

  Expected<int> val(42);
  EXPECT_DEATH(val.error(), "Attempting to access error");

  Expected<void> void_err(internal::ErrorAccess::OutOfMemory());
  EXPECT_DEATH(void_err.value(), "Attempting to access value");

  Expected<void> void_ok;
  EXPECT_DEATH(void_ok.error(), "Attempting to access error");
}
#endif

}  // namespace
}  // namespace hpb
