package com.google.protobuf.kotlin

/**
 * A type meaningful only for its existence, never intended to be instantiated.  For example,
 * a `DslList<Int, FooProxy>` can be given different extension methods than a
 * `DslList<Int, BarProxy>`.
 */
abstract class DslProxy @OnlyForUseByGeneratedProtoCode protected constructor() {
  init {
    throw UnsupportedOperationException("A DslProxy should never be instantiated")
  }
}
