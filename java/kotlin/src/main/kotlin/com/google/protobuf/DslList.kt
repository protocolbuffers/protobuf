// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
