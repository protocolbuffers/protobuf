// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.RetentionPolicy.CLASS;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Annotation for method parameter and class field declarations, which denotes that corresponding
 * actual values must be compile-time constant expressions.
 */
@Documented
@Retention(CLASS)
@Target({ElementType.PARAMETER, ElementType.FIELD})
@interface CompileTimeConstant {}
