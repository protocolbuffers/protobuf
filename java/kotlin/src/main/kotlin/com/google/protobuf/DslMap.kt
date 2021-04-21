package com.google.protobuf.kotlin

/**
 * A simple wrapper around a [Map] with an extra generic parameter that can be used to disambiguate
 * extension methods.
 *
 * <p>This class is used by Kotlin protocol buffer extensions, and its constructor is public only
 * because generated message code is in a different compilation unit.  Others should not use this
 * class directly in any way.
 */
@Suppress("unused") // the unused type parameter
class DslMap<K, V, P : DslProxy> @OnlyForUseByGeneratedProtoCode constructor(
  private val delegate: Map<K, V>
) : Map<K, V> by delegate {
  // We allocate the wrappers on calls to get, not with lazy {...}, because lazy allocates
  // a few objects up front, and any kind of query operation on this object should be rare.

  override val entries: Set<Map.Entry<K, V>>
    get() = UnmodifiableMapEntries(delegate.entries)
  override val keys: Set<K>
    get() = UnmodifiableSet(delegate.keys)
  override val values: Collection<V>
    get() = UnmodifiableCollection(delegate.values)

  override fun equals(other: Any?): Boolean = delegate == other

  override fun hashCode(): Int = delegate.hashCode()

  override fun toString(): String = delegate.toString()
}
