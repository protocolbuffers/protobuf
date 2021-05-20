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
