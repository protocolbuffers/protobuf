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

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.util.AbstractList;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

/**
 * {@code RepeatedFieldBuilderV3} implements a structure that a protocol
 * message uses to hold a repeated field of other protocol messages. It supports
 * the classical use case of adding immutable {@link Message}'s to the
 * repeated field and is highly optimized around this (no extra memory
 * allocations and sharing of immutable arrays).
 * <br>
 * It also supports the additional use case of adding a {@link Message.Builder}
 * to the repeated field and deferring conversion of that {@code Builder}
 * to an immutable {@code Message}. In this way, it's possible to maintain
 * a tree of {@code Builder}'s that acts as a fully read/write data
 * structure.
 * <br>
 * Logically, one can think of a tree of builders as converting the entire tree
 * to messages when build is called on the root or when any method is called
 * that desires a Message instead of a Builder. In terms of the implementation,
 * the {@code SingleFieldBuilderV3} and {@code RepeatedFieldBuilderV3}
 * classes cache messages that were created so that messages only need to be
 * created when some change occurred in its builder or a builder for one of its
 * descendants.
 *
 * @param <MType> the type of message for the field
 * @param <BType> the type of builder for the field
 * @param <IType> the common interface for the message and the builder
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class RepeatedFieldBuilderV3
    <MType extends AbstractMessage,
     BType extends AbstractMessage.Builder,
     IType extends MessageOrBuilder>
    implements AbstractMessage.BuilderParent {

  // Parent to send changes to.
  private AbstractMessage.BuilderParent parent;

  // List of messages. Never null. It may be immutable, in which case
  // isMessagesListMutable will be false. See note below.
  private List<MType> messages;

  // Whether messages is an mutable array that can be modified.
  private boolean isMessagesListMutable;

  // List of builders. May be null, in which case, no nested builders were
  // created. If not null, entries represent the builder for that index.
  private List<SingleFieldBuilderV3<MType, BType, IType>> builders;

  // Here are the invariants for messages and builders:
  // 1. messages is never null and its count corresponds to the number of items
  //    in the repeated field.
  // 2. If builders is non-null, messages and builders MUST always
  //    contain the same number of items.
  // 3. Entries in either array can be null, but for any index, there MUST be
  //    either a Message in messages or a builder in builders.
  // 4. If the builder at an index is non-null, the builder is
  //    authoritative. This is the case where a Builder was set on the index.
  //    Any message in the messages array MUST be ignored.
  // t. If the builder at an index is null, the message in the messages
  //    list is authoritative. This is the case where a Message (not a Builder)
  //    was set directly for an index.

  // Indicates that we've built a message and so we are now obligated
  // to dispatch dirty invalidations. See AbstractMessage.BuilderListener.
  private boolean isClean;

  // A view of this builder that exposes a List interface of messages. This is
  // initialized on demand. This is fully backed by this object and all changes
  // are reflected in it. Access to any item converts it to a message if it
  // was a builder.
  private MessageExternalList<MType, BType, IType> externalMessageList;

  // A view of this builder that exposes a List interface of builders. This is
  // initialized on demand. This is fully backed by this object and all changes
  // are reflected in it. Access to any item converts it to a builder if it
  // was a message.
  private BuilderExternalList<MType, BType, IType> externalBuilderList;

  // A view of this builder that exposes a List interface of the interface
  // implemented by messages and builders. This is initialized on demand. This
  // is fully backed by this object and all changes are reflected in it.
  // Access to any item returns either a builder or message depending on
  // what is most efficient.
  private MessageOrBuilderExternalList<MType, BType, IType>
      externalMessageOrBuilderList;

  /**
   * Constructs a new builder with an empty list of messages.
   *
   * @param messages the current list of messages
   * @param isMessagesListMutable Whether the messages list is mutable
   * @param parent a listener to notify of changes
   * @param isClean whether the builder is initially marked clean
   */
  public RepeatedFieldBuilderV3(
      List<MType> messages,
      boolean isMessagesListMutable,
      AbstractMessage.BuilderParent parent,
      boolean isClean) {
    this.messages = messages;
    this.isMessagesListMutable = isMessagesListMutable;
    this.parent = parent;
    this.isClean = isClean;
  }

  public void dispose() {
    // Null out parent so we stop sending it invalidations.
    parent = null;
  }

  /**
   * Ensures that the list of messages is mutable so it can be updated. If it's
   * immutable, a copy is made.
   */
  private void ensureMutableMessageList() {
    if (!isMessagesListMutable) {
      messages = new ArrayList<MType>(messages);
      isMessagesListMutable = true;
    }
  }

  /**
   * Ensures that the list of builders is not null. If it's null, the list is
   * created and initialized to be the same size as the messages list with
   * null entries.
   */
  private void ensureBuilders() {
    if (this.builders == null) {
      this.builders =
          new ArrayList<SingleFieldBuilderV3<MType, BType, IType>>(
              messages.size());
      for (int i = 0; i < messages.size(); i++) {
        builders.add(null);
      }
    }
  }

  /**
   * Gets the count of items in the list.
   *
   * @return the count of items in the list.
   */
  public int getCount() {
    return messages.size();
  }

  /**
   * Gets whether the list is empty.
   *
   * @return whether the list is empty
   */
  public boolean isEmpty() {
    return messages.isEmpty();
  }

  /**
   * Get the message at the specified index. If the message is currently stored
   * as a {@code Builder}, it is converted to a {@code Message} by
   * calling {@link Message.Builder#buildPartial} on it.
   *
   * @param index the index of the message to get
   * @return the message for the specified index
   */
  public MType getMessage(int index) {
    return getMessage(index, false);
  }

  /**
   * Get the message at the specified index. If the message is currently stored
   * as a {@code Builder}, it is converted to a {@code Message} by
   * calling {@link Message.Builder#buildPartial} on it.
   *
   * @param index the index of the message to get
   * @param forBuild this is being called for build so we want to make sure
   *     we SingleFieldBuilderV3.build to send dirty invalidations
   * @return the message for the specified index
   */
  private MType getMessage(int index, boolean forBuild) {
    if (this.builders == null) {
      // We don't have any builders -- return the current Message.
      // This is the case where no builder was created, so we MUST have a
      // Message.
      return messages.get(index);
    }

    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      // We don't have a builder -- return the current message.
      // This is the case where no builder was created for the entry at index,
      // so we MUST have a message.
      return messages.get(index);

    } else {
      return forBuild ? builder.build() : builder.getMessage();
    }
  }

  /**
   * Gets a builder for the specified index. If no builder has been created for
   * that index, a builder is created on demand by calling
   * {@link Message#toBuilder}.
   *
   * @param index the index of the message to get
   * @return The builder for that index
   */
  public BType getBuilder(int index) {
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      MType message = messages.get(index);
      builder = new SingleFieldBuilderV3<MType, BType, IType>(
          message, this, isClean);
      builders.set(index, builder);
    }
    return builder.getBuilder();
  }

  /**
   * Gets the base class interface for the specified index. This may either be
   * a builder or a message. It will return whatever is more efficient.
   *
   * @param index the index of the message to get
   * @return the message or builder for the index as the base class interface
   */
  @SuppressWarnings("unchecked")
  public IType getMessageOrBuilder(int index) {
    if (this.builders == null) {
      // We don't have any builders -- return the current Message.
      // This is the case where no builder was created, so we MUST have a
      // Message.
      return (IType) messages.get(index);
    }

    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      // We don't have a builder -- return the current message.
      // This is the case where no builder was created for the entry at index,
      // so we MUST have a message.
      return (IType) messages.get(index);

    } else {
      return builder.getMessageOrBuilder();
    }
  }

  /**
   * Sets a  message at the specified index replacing the existing item at
   * that index.
   *
   * @param index the index to set.
   * @param message the message to set
   * @return the builder
   */
  public RepeatedFieldBuilderV3<MType, BType, IType> setMessage(
      int index, MType message) {
    checkNotNull(message);
    ensureMutableMessageList();
    messages.set(index, message);
    if (builders != null) {
      SingleFieldBuilderV3<MType, BType, IType> entry =
          builders.set(index, null);
      if (entry != null) {
        entry.dispose();
      }
    }
    onChanged();
    incrementModCounts();
    return this;
  }

  /**
   * Appends the specified element to the end of this list.
   *
   * @param message the message to add
   * @return the builder
   */
  public RepeatedFieldBuilderV3<MType, BType, IType> addMessage(
      MType message) {
    checkNotNull(message);
    ensureMutableMessageList();
    messages.add(message);
    if (builders != null) {
      builders.add(null);
    }
    onChanged();
    incrementModCounts();
    return this;
  }

  /**
   * Inserts the specified message at the specified position in this list.
   * Shifts the element currently at that position (if any) and any subsequent
   * elements to the right (adds one to their indices).
   *
   * @param index the index at which to insert the message
   * @param message the message to add
   * @return the builder
   */
  public RepeatedFieldBuilderV3<MType, BType, IType> addMessage(
      int index, MType message) {
    checkNotNull(message);
    ensureMutableMessageList();
    messages.add(index, message);
    if (builders != null) {
      builders.add(index, null);
    }
    onChanged();
    incrementModCounts();
    return this;
  }

  /**
   * Appends all of the messages in the specified collection to the end of
   * this list, in the order that they are returned by the specified
   * collection's iterator.
   *
   * @param values the messages to add
   * @return the builder
   */
  public RepeatedFieldBuilderV3<MType, BType, IType> addAllMessages(
      Iterable<? extends MType> values) {
    for (final MType value : values) {
      checkNotNull(value);
    }

    // If we can inspect the size, we can more efficiently add messages.
    int size = -1;
    if (values instanceof Collection) {
      @SuppressWarnings("unchecked") final
      Collection<MType> collection = (Collection<MType>) values;
      if (collection.size() == 0) {
        return this;
      }
      size = collection.size();
    }
    ensureMutableMessageList();

    if (size >= 0 && messages instanceof ArrayList) {
      ((ArrayList<MType>) messages)
          .ensureCapacity(messages.size() + size);
    }

    for (MType value : values) {
      addMessage(value);
    }

    onChanged();
    incrementModCounts();
    return this;
  }

  /**
   * Appends a new builder to the end of this list and returns the builder.
   *
   * @param message the message to add which is the basis of the builder
   * @return the new builder
   */
  public BType addBuilder(MType message) {
    ensureMutableMessageList();
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder =
        new SingleFieldBuilderV3<MType, BType, IType>(
            message, this, isClean);
    messages.add(null);
    builders.add(builder);
    onChanged();
    incrementModCounts();
    return builder.getBuilder();
  }

  /**
   * Inserts a new builder at the specified position in this list.
   * Shifts the element currently at that position (if any) and any subsequent
   * elements to the right (adds one to their indices).
   *
   * @param index the index at which to insert the builder
   * @param message the message to add which is the basis of the builder
   * @return the builder
   */
  public BType addBuilder(int index, MType message) {
    ensureMutableMessageList();
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder =
        new SingleFieldBuilderV3<MType, BType, IType>(
            message, this, isClean);
    messages.add(index, null);
    builders.add(index, builder);
    onChanged();
    incrementModCounts();
    return builder.getBuilder();
  }

  /**
   * Removes the element at the specified position in this list. Shifts any
   * subsequent elements to the left (subtracts one from their indices).
   * Returns the element that was removed from the list.
   *
   * @param index the index at which to remove the message
   */
  public void remove(int index) {
    ensureMutableMessageList();
    messages.remove(index);
    if (builders != null) {
      SingleFieldBuilderV3<MType, BType, IType> entry =
          builders.remove(index);
      if (entry != null) {
        entry.dispose();
      }
    }
    onChanged();
    incrementModCounts();
  }

  /**
   * Removes all of the elements from this list.
   * The list will be empty after this call returns.
   */
  public void clear() {
    messages = Collections.emptyList();
    isMessagesListMutable = false;
    if (builders != null) {
      for (SingleFieldBuilderV3<MType, BType, IType> entry :
          builders) {
        if (entry != null) {
          entry.dispose();
        }
      }
      builders = null;
    }
    onChanged();
    incrementModCounts();
  }

  /**
   * Builds the list of messages from the builder and returns them.
   *
   * @return an immutable list of messages
   */
  public List<MType> build() {
    // Now that build has been called, we are required to dispatch
    // invalidations.
    isClean = true;

    if (!isMessagesListMutable && builders == null) {
      // We still have an immutable list and we never created a builder.
      return messages;
    }

    boolean allMessagesInSync = true;
    if (!isMessagesListMutable) {
      // We still have an immutable list. Let's see if any of them are out
      // of sync with their builders.
      for (int i = 0; i < messages.size(); i++) {
        Message message = messages.get(i);
        SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(i);
        if (builder != null) {
          if (builder.build() != message) {
            allMessagesInSync = false;
            break;
          }
        }
      }
      if (allMessagesInSync) {
        // Immutable list is still in sync.
        return messages;
      }
    }

    // Need to make sure messages is up to date
    ensureMutableMessageList();
    for (int i = 0; i < messages.size(); i++) {
      messages.set(i, getMessage(i, true));
    }

    // We're going to return our list as immutable so we mark that we can
    // no longer update it.
    messages = Collections.unmodifiableList(messages);
    isMessagesListMutable = false;
    return messages;
  }

  /**
   * Gets a view of the builder as a list of messages. The returned list is live
   * and will reflect any changes to the underlying builder.
   *
   * @return the messages in the list
   */
  public List<MType> getMessageList() {
    if (externalMessageList == null) {
      externalMessageList =
          new MessageExternalList<MType, BType, IType>(this);
    }
    return externalMessageList;
  }

  /**
   * Gets a view of the builder as a list of builders. This returned list is
   * live and will reflect any changes to the underlying builder.
   *
   * @return the builders in the list
   */
  public List<BType> getBuilderList() {
    if (externalBuilderList == null) {
      externalBuilderList =
          new BuilderExternalList<MType, BType, IType>(this);
    }
    return externalBuilderList;
  }

  /**
   * Gets a view of the builder as a list of MessageOrBuilders. This returned
   * list is live and will reflect any changes to the underlying builder.
   *
   * @return the builders in the list
   */
  public List<IType> getMessageOrBuilderList() {
    if (externalMessageOrBuilderList == null) {
      externalMessageOrBuilderList =
          new MessageOrBuilderExternalList<MType, BType, IType>(this);
    }
    return externalMessageOrBuilderList;
  }

  /**
   * Called when a the builder or one of its nested children has changed
   * and any parent should be notified of its invalidation.
   */
  private void onChanged() {
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

  /**
   * Increments the mod counts so that an ConcurrentModificationException can
   * be thrown if calling code tries to modify the builder while its iterating
   * the list.
   */
  private void incrementModCounts() {
    if (externalMessageList != null) {
      externalMessageList.incrementModCount();
    }
    if (externalBuilderList != null) {
      externalBuilderList.incrementModCount();
    }
    if (externalMessageOrBuilderList != null) {
      externalMessageOrBuilderList.incrementModCount();
    }
  }

  /**
   * Provides a live view of the builder as a list of messages.
   *
   * @param <MType> the type of message for the field
   * @param <BType> the type of builder for the field
   * @param <IType> the common interface for the message and the builder
   */
  private static class MessageExternalList<
      MType extends AbstractMessage,
      BType extends AbstractMessage.Builder,
      IType extends MessageOrBuilder>
      extends AbstractList<MType> implements List<MType> {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    MessageExternalList(
        RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    @Override
    public int size() {
      return this.builder.getCount();
    }

    @Override
    public MType get(int index) {
      return builder.getMessage(index);
    }

    void incrementModCount() {
      modCount++;
    }
  }

  /**
   * Provides a live view of the builder as a list of builders.
   *
   * @param <MType> the type of message for the field
   * @param <BType> the type of builder for the field
   * @param <IType> the common interface for the message and the builder
   */
  private static class BuilderExternalList<
      MType extends AbstractMessage,
      BType extends AbstractMessage.Builder,
      IType extends MessageOrBuilder>
      extends AbstractList<BType> implements List<BType> {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    BuilderExternalList(
        RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    @Override
    public int size() {
      return this.builder.getCount();
    }

    @Override
    public BType get(int index) {
      return builder.getBuilder(index);
    }

    void incrementModCount() {
      modCount++;
    }
  }

  /**
   * Provides a live view of the builder as a list of builders.
   *
   * @param <MType> the type of message for the field
   * @param <BType> the type of builder for the field
   * @param <IType> the common interface for the message and the builder
   */
  private static class MessageOrBuilderExternalList<
      MType extends AbstractMessage,
      BType extends AbstractMessage.Builder,
      IType extends MessageOrBuilder>
      extends AbstractList<IType> implements List<IType> {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    MessageOrBuilderExternalList(
        RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    @Override
    public int size() {
      return this.builder.getCount();
    }

    @Override
    public IType get(int index) {
      return builder.getMessageOrBuilder(index);
    }

    void incrementModCount() {
      modCount++;
    }
  }
}
