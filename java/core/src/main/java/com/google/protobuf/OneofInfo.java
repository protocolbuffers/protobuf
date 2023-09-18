// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.lang.reflect.Field;

/** Information for a oneof within a protobuf message. */
// TODO: make this private once all of experimental code is migrated to protobuf.
@ExperimentalApi
@CheckReturnValue
final class OneofInfo {
  private final int id;
  private final Field caseField;
  private final Field valueField;

  public OneofInfo(int id, Field caseField, Field valueField) {
    this.id = id;
    this.caseField = caseField;
    this.valueField = valueField;
  }

  /**
   * Returns the unique identifier of the oneof within the message. This is really just an index
   * starting at zero.
   */
  public int getId() {
    return id;
  }

  /** The {@code int} field containing the field number of the currently active member. */
  public Field getCaseField() {
    return caseField;
  }

  /** The {@link Object} field containing the value of the currently active member. */
  public Field getValueField() {
    return valueField;
  }
}
