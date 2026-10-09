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
 * Identifies the {@link Object} field within a protobuf message that contains the value of a oneof.
 */
@ExperimentalApi
@Retention(CLASS)
@Target(FIELD)
public @interface ProtoOneof {
  /** Returns the index of this oneof within the parent message. */
  int index();

  /** Returns the field numbers for each member of this oneof. */
  int[] fieldNumbers();

  /** Returns the {@link FieldType} for each member of this oneof. */
  FieldType[] types();

  /**
   * Returns the actual type stored in the oneof value for each member. Since the oneof value is an
   * {@link Object}, primitives will store their boxed type.
   */
  Class<?>[] storedTypes();
}
