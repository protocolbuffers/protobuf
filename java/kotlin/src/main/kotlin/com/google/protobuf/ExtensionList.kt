package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.MessageLite

/**
 * Implementation for ExtensionList and ExtensionListLite.  Like [DslList], represents an
 * unmodifiable view of a repeated proto field -- in this case, an extension field -- but
 * supports querying the extension.
 */
class ExtensionList<E, M : MessageLite> @OnlyForUseByGeneratedProtoCode constructor(
  val extension: ExtensionLite<M, List<E>>,
  @JvmField private val delegate: List<E>
) : List<E> by delegate {
  override fun iterator(): Iterator<E> = UnmodifiableIterator(delegate.iterator())

  override fun listIterator(): ListIterator<E> = UnmodifiableListIterator(delegate.listIterator())

  override fun listIterator(index: Int): ListIterator<E> =
    UnmodifiableListIterator(delegate.listIterator(index))

  override fun equals(other: Any?): Boolean = delegate == other

  override fun hashCode(): Int = delegate.hashCode()

  override fun toString(): String = delegate.toString()
}
