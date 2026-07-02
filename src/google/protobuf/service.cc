// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/service.h"

#include <functional>
#include <utility>

#include "google/protobuf/stubs/callback.h"
#include "google/protobuf/stubs/common.h"

namespace google {
namespace protobuf {

Service::~Service() {}
RpcChannel::~RpcChannel() {}
RpcController::~RpcController() {}

namespace {

// A Closure that wraps a std::function<void()> and deletes itself on call.
class FunctionClosure final : public Closure {
 public:
  explicit FunctionClosure(std::function<void()> f) : f_(std::move(f)) {}
  void Run() override {
    f_();
    delete this;
  }

 private:
  std::function<void()> f_;
};

}  // namespace

Closure* ToClosure(std::function<void()> f) {
  if (f == nullptr) {
    return nullptr;
  }
  return new FunctionClosure(std::move(f));
}

}  // namespace protobuf
}  // namespace google
