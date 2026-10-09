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

/** Represents field level options for a Protocol Buffer message. This is used by Proguard. */
@ExperimentalApi
@Target(FIELD)
@Retention(RetentionPolicy.CLASS)
public @interface ProtoField {
  /**
   * Return if this field is required.
   *
   * <p>Note: This only applies to proto2 messages. For syntax > {@link ProtoSyntax#PROTO2}, all
   * fields are optional.
   */
  boolean isRequired() default false;
}
