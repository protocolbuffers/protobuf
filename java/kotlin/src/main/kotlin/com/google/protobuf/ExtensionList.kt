// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.MessageLite

private val UNINITIALIZED_SUPPLIER: () -> Nothing = {
  throw IllegalStateException("Should not be called")
}

/**
 * Implementation for ExtensionList and ExtensionListLite. Like [DslList], represents an
 * unmodifiable view of a repeated proto field -- in this case, an extension field -- but supports
 * querying the extension.
 */
class ExtensionList<E, M : MessageLite>
@OnlyForUseByGeneratedProtoCode
constructor(
  val extension: ExtensionLite<M, List<E>>,
  private val delegateSupplier: () -> List<E>,
) : List<E> {
  @Suppress("UNCHECKED_CAST")
  @OnlyForUseByGeneratedProtoCode
  constructor(
    extension: ExtensionLite<M, List<E>>,
    delegate: List<E>,
  ) : this(extension, UNINITIALIZED_SUPPLIER as () -> List<E>) {
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
