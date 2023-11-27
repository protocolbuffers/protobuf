// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

@CheckReturnValue
final class MapFieldSchemas {
  private static final MapFieldSchema FULL_SCHEMA = loadSchemaForFullRuntime();
  private static final MapFieldSchema LITE_SCHEMA = new MapFieldSchemaLite();

  static MapFieldSchema full() {
    return FULL_SCHEMA;
  }

  static MapFieldSchema lite() {
    return LITE_SCHEMA;
  }

  private static MapFieldSchema loadSchemaForFullRuntime() {
    try {
      Class<?> clazz = Class.forName("com.google.protobuf.MapFieldSchemaFull");
      return (MapFieldSchema) clazz.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      return null;
    }
  }
}
