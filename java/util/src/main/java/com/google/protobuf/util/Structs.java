// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import com.google.protobuf.Struct;
import com.google.protobuf.Value;

/** Utilities to help create {@code google.protobuf.Struct} messages. */
public final class Structs {

  /**
   * Returns a struct containing the key-value pair.
   */
  public static Struct of(String k1, Value v1) {
    return Struct.newBuilder().putFields(k1, v1).build();
  }

  /**
   * Returns a struct containing each of the key-value pairs.
   *
   * <p>Providing duplicate keys is undefined behavior.
   */
  public static Struct of(String k1, Value v1, String k2, Value v2) {
    return Struct.newBuilder().putFields(k1, v1).putFields(k2, v2).build();
  }

  /**
   * Returns a struct containing each of the key-value pairs.
   *
   * <p>Providing duplicate keys is undefined behavior.
   */
  public static Struct of(String k1, Value v1, String k2, Value v2, String k3, Value v3) {
    return Struct.newBuilder().putFields(k1, v1).putFields(k2, v2).putFields(k3, v3).build();
  }

  private Structs() {}
}
