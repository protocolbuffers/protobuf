// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.ElementType.FIELD;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Indicates that the marked field is a proto2 presence-checked field. */
@Target(FIELD)
@Retention(RetentionPolicy.CLASS)
public @interface ProtoPresenceCheckedField {
  /** The ID of the {@link ProtoPresenceBits} field. */
  int presenceBitsId();

  /** The mask used to check the presence bit for this field. */
  int mask();
}
