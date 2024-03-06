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

namespace google {
namespace protobuf {

Service::~Service() {}
RpcChannel::~RpcChannel() {}
RpcController::~RpcController() {}

}  // namespace protobuf
}  // namespace google
