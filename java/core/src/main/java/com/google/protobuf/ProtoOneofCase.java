// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.CLASS;

import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Identifies the {@code int} field within a protobuf message that contains the case value for a
 * given oneof.
 */
@ExperimentalApi
@Retention(CLASS)
@Target(FIELD)
public @interface ProtoOneofCase {
  /**
   * Returns the index of the oneof (relative to the enclosing message) to which this case field
   * refers.
   */
  int oneofIndex();
}
