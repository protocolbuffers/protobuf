// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

private val UNINITIALIZED_SUPPLIER: () -> Nothing = {
  throw IllegalStateException("Should not be called")
}

/**
 * A simple wrapper around a [List] with an extra generic parameter that can be used to disambiguate
 * extension methods.
 *
 * <p>This class is used by Kotlin protocol buffer extensions, and its constructor is public only
 * because generated message code is in a different compilation unit. Others should not use this
 * class directly in any way.
 */
@Suppress("unused") // the unused type parameter
class DslList<E, P : DslProxy>
@OnlyForUseByGeneratedProtoCode
constructor(private val delegateSupplier: () -> List<E>) : List<E> {
  @Suppress("UNCHECKED_CAST")
  @OnlyForUseByGeneratedProtoCode
  constructor(delegate: List<E>) : this(UNINITIALIZED_SUPPLIER as () -> List<E>) {
    memoizedDelegate = delegate
  }

  private var memoizedDelegate: List<E>? = null

  private val delegate: List<E>
    get() {
      var result = memoizedDelegate
      if (result == null) {
        result = delegateSupplier()
        memoizedDelegate = result
      }
      return result
    }

  override val size: Int
    get() = delegate.size

  override fun isEmpty(): Boolean = delegate.isEmpty()

  override fun contains(element: E): Boolean = delegate.contains(element)

  override fun containsAll(elements: Collection<E>): Boolean = delegate.containsAll(elements)

  override fun get(index: Int): E = delegate[index]

  override fun indexOf(element: E): Int = delegate.indexOf(element)

  override fun lastIndexOf(element: E): Int = delegate.lastIndexOf(element)

  override fun subList(fromIndex: Int, toIndex: Int): List<E> = delegate.subList(fromIndex, toIndex)

  override fun iterator(): Iterator<E> = UnmodifiableIterator(delegate.iterator())

  override fun listIterator(): ListIterator<E> = UnmodifiableListIterator(delegate.listIterator())

  override fun listIterator(index: Int): ListIterator<E> =
    UnmodifiableListIterator(delegate.listIterator(index))

  override fun equals(other: Any?): Boolean = delegate == other

  override fun hashCode(): Int = delegate.hashCode()

  override fun toString(): String = delegate.toString()
}

// TODO: investigate making this an inline class
