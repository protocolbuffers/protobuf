// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

/**
 * Opt-in annotation to make it difficult to accidentally use APIs only intended for use by proto
 * generated code. See https://kotlinlang.org/docs/reference/opt-in-requirements.html for details on
 * how this API works.
 */
@RequiresOptIn(
  message =
    """
    This API is only intended for use by generated protobuf code, the code generator, and their own
    tests.  If this does not describe your code, you should not be using this API.
  """,
  level = RequiresOptIn.Level.ERROR,
)
@Retention(AnnotationRetention.BINARY)
@Target(AnnotationTarget.CONSTRUCTOR, AnnotationTarget.ANNOTATION_CLASS)
annotation class OnlyForUseByGeneratedProtoCode
