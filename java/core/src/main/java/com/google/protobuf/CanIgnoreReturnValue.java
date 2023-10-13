// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.ElementType.METHOD;
import static java.lang.annotation.RetentionPolicy.CLASS;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Indicates that the return value of the annotated method can be safely ignored.
 *
 * <p>This is the opposite of {@link CheckReturnValue}. It can be used inside classes or packages
 * annotated with {@code @CheckReturnValue} to exempt specific methods from the default.
 */
@Documented
@Target(METHOD) // TODO: consider adding CONSTRUCTOR later if necessary
@Retention(CLASS)
@interface CanIgnoreReturnValue {}
