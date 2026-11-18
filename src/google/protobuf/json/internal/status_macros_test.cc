// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/json/internal/status_macros.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"

namespace google {
namespace protobuf {
namespace json_internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;

TEST(StatusMacrosTest, ReturnIfErrorPassesOnOkStatus) {
  int calls = 0;
  auto fn = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR([&] {
      ++calls;
      return absl::OkStatus();
    }());
    const absl::Status ok = absl::OkStatus();
    PB_RETURN_IF_ERROR(ok);
    absl::Status mutable_ok = absl::OkStatus();
    PB_RETURN_IF_ERROR(mutable_ok);
    return absl::OkStatus();
  };

  EXPECT_THAT(fn(), IsOk());
  EXPECT_EQ(calls, 1);
}

TEST(StatusMacrosTest, ReturnIfErrorPropagatesErrorStatus) {
  int calls = 0;
  auto from_rvalue = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR([&] {
      ++calls;
      return absl::InvalidArgumentError("bad arg");
    }());
    return absl::OkStatus();
  };
  EXPECT_THAT(from_rvalue(),
              StatusIs(absl::StatusCode::kInvalidArgument, "bad arg"));
  EXPECT_EQ(calls, 1);

  const absl::Status const_err = absl::PermissionDeniedError("denied");
  auto from_const_lvalue = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR(const_err);
    return absl::OkStatus();
  };
  EXPECT_THAT(from_const_lvalue(),
              StatusIs(absl::StatusCode::kPermissionDenied, "denied"));
  EXPECT_THAT(const_err,
              StatusIs(absl::StatusCode::kPermissionDenied, "denied"));

  absl::Status mutable_err = absl::NotFoundError("missing");
  auto from_lvalue = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR(mutable_err);
    return absl::OkStatus();
  };
  EXPECT_THAT(from_lvalue(), StatusIs(absl::StatusCode::kNotFound, "missing"));
}

TEST(StatusMacrosTest, ReturnIfErrorWorksWithStatusOr) {
  auto ok_fn = []() -> absl::Status {
    PB_RETURN_IF_ERROR(absl::StatusOr<int>(42));
    const absl::StatusOr<std::string> ok_str("hello");
    PB_RETURN_IF_ERROR(ok_str);
    return absl::OkStatus();
  };
  EXPECT_THAT(ok_fn(), IsOk());

  int calls = 0;
  auto err_rvalue = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR([&]() -> absl::StatusOr<std::string> {
      ++calls;
      return absl::DataLossError("corrupt");
    }());
    return absl::OkStatus();
  };
  EXPECT_THAT(err_rvalue(), StatusIs(absl::StatusCode::kDataLoss, "corrupt"));
  EXPECT_EQ(calls, 1);

  absl::StatusOr<int> err_lvalue = absl::InternalError("boom");
  auto err_from_lvalue = [&]() -> absl::Status {
    PB_RETURN_IF_ERROR(err_lvalue);
    return absl::OkStatus();
  };
  EXPECT_THAT(err_from_lvalue(), StatusIs(absl::StatusCode::kInternal, "boom"));
}

TEST(StatusMacrosTest, ReturnIfErrorIfElseSafe) {
  auto fn = [](bool cond) -> absl::Status {
    bool else_ran = false;
    if (cond)
      PB_RETURN_IF_ERROR(absl::OkStatus());
    else
      else_ran = true;

    if (cond) {
      EXPECT_FALSE(else_ran);
    } else {
      EXPECT_TRUE(else_ran);
    }
    return absl::OkStatus();
  };

  EXPECT_THAT(fn(true), IsOk());
  EXPECT_THAT(fn(false), IsOk());
}

TEST(StatusMacrosTest, DefineOrReturnAssignsScalars) {
  int target = 123;
  int calls = 0;
  auto fn = [&]() -> absl::Status {
    const auto get_seven = [&]() -> absl::StatusOr<int> {
      ++calls;
      return 7;
    };
    PB_DEFINE_OR_RETURN(a, get_seven());
    const absl::StatusOr<int*> ptr_sor(&target);
    PB_DEFINE_OR_RETURN(p, ptr_sor);
    absl::StatusOr<bool> bool_sor(true);
    PB_DEFINE_OR_RETURN(b, bool_sor);

    EXPECT_EQ(a, 7);
    EXPECT_EQ(*p, 123);
    EXPECT_TRUE(b);
    return absl::OkStatus();
  };

  EXPECT_THAT(fn(), IsOk());
  EXPECT_EQ(calls, 1);
}

TEST(StatusMacrosTest, DefineOrReturnPropagatesError) {
  bool reached_after_error = false;
  auto fn = [&]() -> absl::Status {
    PB_DEFINE_OR_RETURN(x, absl::StatusOr<int>(absl::OutOfRangeError("oob")));
    (void)x;
    reached_after_error = true;
    return absl::OkStatus();
  };

  EXPECT_THAT(fn(), StatusIs(absl::StatusCode::kOutOfRange, "oob"));
  EXPECT_FALSE(reached_after_error);
}

}  // namespace
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
