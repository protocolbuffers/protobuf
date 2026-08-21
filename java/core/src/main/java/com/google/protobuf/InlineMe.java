// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.annotation.ElementType.CONSTRUCTOR;
import static java.lang.annotation.ElementType.METHOD;

import java.lang.annotation.Documented;
import java.lang.annotation.Target;

/**
 * Indicates that callers of this API should be inlined. That is, this API is trivially expressible
 * in terms of another API, for example a method that just calls another method.
 */
@Documented
@Target({METHOD, CONSTRUCTOR})
@interface InlineMe {
  /**
   * What the caller should be replaced with. Local parameter names can be used in the replacement
   * string. If you are invoking an instance method or constructor, you must include the implicit
   * {@code this} in the replacement body. If you are invoking a static method, you must include the
   * implicit {@code ClassName} in the replacement body.
   */
  String replacement();

  /** The new imports to (optionally) add to the caller. */
  String[] imports() default {};

  /** The new static imports to (optionally) add to the caller. */
  String[] staticImports() default {};
}
