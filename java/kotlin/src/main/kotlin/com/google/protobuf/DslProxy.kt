// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

/**
 * A type meaningful only for its existence, never intended to be instantiated. For example, a
 * `DslList<Int, FooProxy>` can be given different extension methods than a `DslList<Int,
 * BarProxy>`.
 */
abstract class DslProxy @OnlyForUseByGeneratedProtoCode protected constructor() {
  init {
    throw UnsupportedOperationException("A DslProxy should never be instantiated")
  }
}
