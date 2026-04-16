// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Thrown when a protocol message being parsed is invalid in some way, but a checked exception is
 * not allowed. For instance, in a lazy field value getter.
 */
public class InvalidProtobufRuntimeException extends RuntimeException {
  private static final long serialVersionUID = 1L;

  public InvalidProtobufRuntimeException(String description) {
    super(description);
  }

  public InvalidProtobufRuntimeException(Exception e) {
    super(e.getMessage(), e);
  }

  public InvalidProtobufRuntimeException(String description, Exception e) {
    super(description, e);
  }
}
