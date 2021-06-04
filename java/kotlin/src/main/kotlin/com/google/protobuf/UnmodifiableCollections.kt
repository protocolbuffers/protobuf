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

/** Wraps an [Iterator] and makes it unmodifiable even from Java. */
internal class UnmodifiableIterator<E>(delegate: Iterator<E>) : Iterator<E> by delegate

/** Wraps a [ListIterator] and makes it unmodifiable even from Java. */
internal class UnmodifiableListIterator<E>(
  delegate: ListIterator<E>
) : ListIterator<E> by delegate

/** Wraps a [Collection] and makes it unmodifiable even from Java. */
internal open class UnmodifiableCollection<E>(
  private val delegate: Collection<E>
) : Collection<E> by delegate {
  override fun iterator(): Iterator<E> = UnmodifiableIterator(delegate.iterator())
}

/** Wraps a [Set] and makes it unmodifiable even from Java. */
internal class UnmodifiableSet<E>(
  delegate: Collection<E>
) : UnmodifiableCollection<E>(delegate), Set<E>

/** Wraps a [Map.Entry] and makes it unmodifiable even from Java. */
internal class UnmodifiableMapEntry<K, V>(delegate: Map.Entry<K, V>) : Map.Entry<K, V> by delegate

/** Wraps a [Set] of map entries and makes it unmodifiable even from Java. */
internal class UnmodifiableMapEntries<K, V>(
  private val delegate: Set<Map.Entry<K, V>>
) : UnmodifiableCollection<Map.Entry<K, V>>(delegate), Set<Map.Entry<K, V>> {

  // Is this overkill? Probably.

  override fun iterator(): Iterator<Map.Entry<K, V>> {
    val itr = delegate.iterator()
    return object : Iterator<Map.Entry<K, V>> by itr {
      override fun next(): Map.Entry<K, V> = UnmodifiableMapEntry(itr.next())
    }
  }
}
