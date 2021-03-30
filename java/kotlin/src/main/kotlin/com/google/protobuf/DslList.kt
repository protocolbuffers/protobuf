package com.google.protobuf.kotlin

/**
 * A simple wrapper around a [List] with an extra generic parameter that can be used to disambiguate
 * extension methods.
 *
 * <p>This class is used by Kotlin protocol buffer extensions, and its constructor is public only
 * because generated message code is in a different compilation unit.  Others should not use this
 * class directly in any way.
 */
@Suppress("unused") // the unused type parameter
class DslList<E, P : DslProxy> @OnlyForUseByGeneratedProtoCode constructor(
  private val delegate: List<E>
) : List<E> by delegate {
  override fun iterator(): Iterator<E> = UnmodifiableIterator(delegate.iterator())

  override fun listIterator(): ListIterator<E> = UnmodifiableListIterator(delegate.listIterator())

  override fun listIterator(index: Int): ListIterator<E> =
    UnmodifiableListIterator(delegate.listIterator(index))

  override fun equals(other: Any?): Boolean = delegate == other

  override fun hashCode(): Int = delegate.hashCode()

  override fun toString(): String = delegate.toString()
}
