// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

final class NewInstanceSchemaFull implements NewInstanceSchema {
  @Override
  public Object newInstance(Object defaultInstance) {
    return ((Message) defaultInstance).toBuilder().buildPartial();
  }
}
