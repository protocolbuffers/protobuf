// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.ElementType.CONSTRUCTOR;
import static java.lang.annotation.ElementType.METHOD;
import static java.lang.annotation.ElementType.PACKAGE;
import static java.lang.annotation.ElementType.TYPE;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Indicates that the return value of the annotated method must be used. An error is triggered when
 * one of these methods is called but the result is not used.
 *
 * <p>{@code @CheckReturnValue} may be applied to a class or package to indicate that all methods in
 * that class (including indirectly; that is, methods of inner classes within the annotated class)
 * or package must have their return values used. For convenience, we provide an annotation, {@link
 * CanIgnoreReturnValue}, to exempt specific methods or classes from this behavior.
 */
@Documented
@Target({METHOD, CONSTRUCTOR, TYPE, PACKAGE})
@Retention(RUNTIME)
@interface CheckReturnValue {}
