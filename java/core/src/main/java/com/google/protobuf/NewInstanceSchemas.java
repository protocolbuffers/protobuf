// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

@CheckReturnValue
final class NewInstanceSchemas {
  private static final NewInstanceSchema FULL_SCHEMA = loadSchemaForFullRuntime();
  private static final NewInstanceSchema LITE_SCHEMA = new NewInstanceSchemaLite();

  static NewInstanceSchema full() {
    return FULL_SCHEMA;
  }

  static NewInstanceSchema lite() {
    return LITE_SCHEMA;
  }

  private static NewInstanceSchema loadSchemaForFullRuntime() {
    try {
      Class<?> clazz = Class.forName("com.google.protobuf.NewInstanceSchemaFull");
      return (NewInstanceSchema) clazz.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      return null;
    }
  }
}
