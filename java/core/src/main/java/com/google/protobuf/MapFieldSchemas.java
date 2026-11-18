// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

@CheckReturnValue
final class MapFieldSchemas {
  private static final MapFieldSchema LITE_SCHEMA = new MapFieldSchemaLite();

  static MapFieldSchema lite() {
    return LITE_SCHEMA;
  }

  private MapFieldSchemas() {}
}
