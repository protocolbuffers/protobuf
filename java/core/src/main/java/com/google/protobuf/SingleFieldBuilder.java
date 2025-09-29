// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

/**
 * {@code SingleFieldBuilder} implements a structure that a protocol message uses to hold a single
 * field of another protocol message. It supports the classical use case of setting an immutable
 * {@link Message} as the value of the field and is highly optimized around this.
 *
 * <p>It also supports the additional use case of setting a {@link Message.Builder} as the field and
 * deferring conversion of that {@code Builder} to an immutable {@code Message}. In this way, it's
 * possible to maintain a tree of {@code Builder}'s that acts as a fully read/write data structure.
 * <br>
 * Logically, one can think of a tree of builders as converting the entire tree to messages when
 * build is called on the root or when any method is called that desires a Message instead of a
 * Builder. In terms of the implementation, the {@code SingleFieldBuilder} and {@code
 * RepeatedFieldBuilder} classes cache messages that were created so that messages only need to be
 * created when some change occurred in its builder or a builder for one of its descendants.
 *
 * @param <MType> the type of message for the field
 * @param <BType> the type of builder for the field
 * @param <IType> the common interface for the message and the builder
 * @author jonp@google.com (Jon Perlow)
 */
public class SingleFieldBuilder<
        MType extends GeneratedMessage,
        BType extends GeneratedMessage.Builder,
        IType extends MessageOrBuilder>
    implements GeneratedMessage.BuilderParent {

  // Parent to send changes to.
  private GeneratedMessage.BuilderParent parent;

  // Invariant: one of builder or message fields must be non-null.

  // If set, this is the case where we are backed by a builder. In this case,
  // message field represents a cached message for the builder (or null if
  // there is no cached message).
  private BType builder;

  // If builder is non-null, this represents a cached message from the builder.
  // If builder is null, this is the authoritative message for the field.
  private MType message;

  // Indicates that we've built a message and so we are now obligated
  // to dispatch dirty invalidations. See GeneratedMessage.BuilderListener.
  private boolean isClean;

  public SingleFieldBuilder(MType message, GeneratedMessage.BuilderParent parent, boolean isClean) {
    this.message = checkNotNull(message);
    this.parent = parent;
    this.isClean = isClean;
  }

  public void dispose() {
    // Null out parent so we stop sending it invalidations.
    parent = null;
  }

  /**
   * Get the message for the field. If the message is currently stored as a {@code Builder}, it is
   * converted to a {@code Message} by calling {@link Message.Builder#buildPartial} on it. If no
   * message has been set, returns the default instance of the message.
   *
   * @return the message for the field
   */
  @SuppressWarnings("unchecked")
  public MType getMessage() {
    if (message == null) {
      // If message is null, the invariant is that we must be have a builder.
      message = (MType) builder.buildPartial();
    }
    return message;
  }

  /**
   * Builds the message and returns it.
   *
   * @return the message
   */
  public MType build() {
    // Now that build has been called, we are required to dispatch
    // invalidations.
    isClean = true;
    return getMessage();
  }

  /**
   * Gets a builder for the field. If no builder has been created yet, a builder is created on
   * demand by calling {@link Message#toBuilder}.
   *
   * @return The builder for the field
   */
  @SuppressWarnings("unchecked")
  public BType getBuilder() {
    // This code is very hot.
    // Optimisation: store this.builder in a local variable so that the compiler doesn't reload
    // it at 'return'. Android's compiler thinks the methods called between assignment & return
    // might reassign this.builder so the compiler can't make this optimisation without our help.
    BType builder = this.builder;

    if (builder == null) {
      // builder.mergeFrom() on a fresh builder
      // does not create any sub-objects with independent clean/dirty states,
      // therefore setting the builder itself to clean without actually calling
      // build() cannot break any invariants.
      this.builder = builder = (BType) message.newBuilderForType(this);
      builder.mergeFrom(message); // no-op if message is the default message
      builder.markClean();
    }
    return builder;
  }

  /**
   * Gets the base class interface for the field. This may either be a builder or a message. It will
   * return whatever is more efficient.
   *
   * @return the message or builder for the field as the base class interface
   */
  @SuppressWarnings("unchecked")
  public IType getMessageOrBuilder() {
    if (builder != null) {
      return (IType) builder;
    } else {
      return (IType) message;
    }
  }

  /**
   * Sets a message for the field replacing any existing value.
   *
   * @param message the message to set
   * @return the builder
   */
  @CanIgnoreReturnValue
  public SingleFieldBuilder<MType, BType, IType> setMessage(MType message) {
    this.message = checkNotNull(message);
    if (builder != null) {
      builder.dispose();
      builder = null;
    }
    onChanged();
    return this;
  }

  /**
   * Merges the field from another field.
   *
   * @param value the value to merge from
   * @return the builder
   */
  @CanIgnoreReturnValue
  public SingleFieldBuilder<MType, BType, IType> mergeFrom(MType value) {
    if (builder == null && message == message.getDefaultInstanceForType()) {
      message = value;
    } else {
      getBuilder().mergeFrom(value);
    }
    onChanged();
    return this;
  }

  /**
   * Clears the value of the field.
   *
   * @return the builder
   */
  @SuppressWarnings("unchecked")
  @CanIgnoreReturnValue
  public SingleFieldBuilder<MType, BType, IType> clear() {
    message =
        (MType)
            (message != null
                ? message.getDefaultInstanceForType()
                : builder.getDefaultInstanceForType());
    if (builder != null) {
      builder.dispose();
      builder = null;
    }
    onChanged();
    // After clearing, parent is dirty, but this field builder is now clean and any changes should
    // trickle up.
    isClean = true;
    return this;
  }

  /**
   * Called when a the builder or one of its nested children has changed and any parent should be
   * notified of its invalidation.
   */
  private void onChanged() {
    // If builder is null, this is the case where onChanged is being called
    // from setMessage or clear.
    if (builder != null) {
      message = null;
    }
    if (isClean && parent != null) {
      parent.markDirty();

      // Don't keep dispatching invalidations until build is called again.
      isClean = false;
    }
  }

  @Override
  public void markDirty() {
    onChanged();
  }
}
