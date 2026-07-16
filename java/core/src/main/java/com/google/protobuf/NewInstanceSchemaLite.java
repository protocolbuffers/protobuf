// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * This class is for Lite runtime use only.
 *
 * <p>For details on what this means regarding performance and security characteristics, see {@link
 * ForLiteOnly}.
 */
@CheckReturnValue
@ForLiteOnly
final class NewInstanceSchemaLite implements NewInstanceSchema {
  @Override
  public Object newInstance(Object defaultInstance) {
    // TODO decide if we're keeping support for Full in schema classes and handle this
    // better.
    return ((GeneratedMessageLite<?, ?>) defaultInstance).newMutableInstance();
  }
}
