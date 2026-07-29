// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

@CheckReturnValue
final class ListFieldSchemas {
  private static final ListFieldSchema LITE_SCHEMA = new ListFieldSchemaLite();

  static ListFieldSchema lite() {
    return LITE_SCHEMA;
  }

  private ListFieldSchemas() {}
}
