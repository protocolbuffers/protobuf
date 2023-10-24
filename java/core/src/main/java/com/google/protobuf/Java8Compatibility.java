// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.nio.Buffer;

/**
 * Wrappers around {@link Buffer} methods that are covariantly overridden in Java 9+. See
 * https://github.com/protocolbuffers/protobuf/issues/11393
 *
 * <p>TODO remove when Java 8 support is no longer needed.
 */
final class Java8Compatibility {
  static void clear(Buffer b) {
    b.clear();
  }

  static void flip(Buffer b) {
    b.flip();
  }

  static void limit(Buffer b, int limit) {
    b.limit(limit);
  }

  static void mark(Buffer b) {
    b.mark();
  }

  static void position(Buffer b, int position) {
    b.position(position);
  }

  static void reset(Buffer b) {
    b.reset();
  }

  private Java8Compatibility() {}
}
