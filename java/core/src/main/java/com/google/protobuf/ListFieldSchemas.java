// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

@CheckReturnValue
final class ListFieldSchemas {
  private static final ListFieldSchema FULL_SCHEMA = loadSchemaForFullRuntime();
  private static final ListFieldSchema LITE_SCHEMA = new ListFieldSchemaLite();

  static ListFieldSchema full() {
    return FULL_SCHEMA;
  }

  static ListFieldSchema lite() {
    return LITE_SCHEMA;
  }

  private static ListFieldSchema loadSchemaForFullRuntime() {
    if (Protobuf.assumeLiteRuntime) {
      return null;
    }
    try {
      Class<?> clazz = Class.forName("com.google.protobuf.ListFieldSchemaFull");
      return (ListFieldSchema) clazz.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      return null;
    }
  }

  private ListFieldSchemas() {}
}
