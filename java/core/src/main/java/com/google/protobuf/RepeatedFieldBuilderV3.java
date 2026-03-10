// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.util.AbstractList;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.RandomAccess;

/**
 * Stub for RepeatedFieldBuilderV3 matching RepeatedFieldBuilder for compatibility with older
 * gencode. This cannot wrap RepeatedFieldBuilder directly due to RepeatedFieldBuilder having more
 * restrictive extends GeneratedMessage for MType and BType.
 *
 * @deprecated This class is deprecated, and slated for removal in the next breaking change. Users
 *     should update gencode to >= 4.26.x which replaces RepeatedFieldBuilderV3 with
 *     RepeatedFieldBuilder.
 */
@Deprecated
public class RepeatedFieldBuilderV3<
        MType extends AbstractMessage,
        BType extends AbstractMessage.Builder,
        IType extends MessageOrBuilder>
    implements AbstractMessage.BuilderParent {

  private AbstractMessage.BuilderParent parent;

  private List<MType> messages;

  private boolean isMessagesListMutable;

  private List<SingleFieldBuilderV3<MType, BType, IType>> builders;

  private boolean isClean;

  private MessageExternalList<MType, BType, IType> externalMessageList;

  private BuilderExternalList<MType, BType, IType> externalBuilderList;

  private MessageOrBuilderExternalList<MType, BType, IType> externalMessageOrBuilderList;

  @Deprecated
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

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.dispose() instead.
   */
  @Deprecated
  public void dispose() {
    parent = null;
  }

  private void ensureMutableMessageList() {
    if (!isMessagesListMutable) {
      messages = new ArrayList<MType>(messages);
      isMessagesListMutable = true;
    }
  }

  private void ensureBuilders() {
    if (this.builders == null) {
      this.builders = new ArrayList<SingleFieldBuilderV3<MType, BType, IType>>(messages.size());
      for (int i = 0; i < messages.size(); i++) {
        builders.add(null);
      }
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getCount() instead.
   */
  @Deprecated
  public int getCount() {
    return messages.size();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.isEmpty() instead.
   */
  @Deprecated
  public boolean isEmpty() {
    return messages.isEmpty();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getMessage() instead.
   */
  @Deprecated
  public MType getMessage(int index) {
    return getMessage(index, false);
  }

  private MType getMessage(int index, boolean forBuild) {
    if (this.builders == null) {
      return messages.get(index);
    }

    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      return messages.get(index);

    } else {
      return forBuild ? builder.build() : builder.getMessage();
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getBuilder() instead.
   */
  @Deprecated
  public BType getBuilder(int index) {
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      MType message = messages.get(index);
      builder = new SingleFieldBuilderV3<MType, BType, IType>(message, this, isClean);
      builders.set(index, builder);
    }
    return builder.getBuilder();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getMessageOrBuilder() instead.
   */
  @Deprecated
  @SuppressWarnings("unchecked")
  public IType getMessageOrBuilder(int index) {
    if (this.builders == null) {
      return (IType) messages.get(index);
    }

    SingleFieldBuilderV3<MType, BType, IType> builder = builders.get(index);
    if (builder == null) {
      return (IType) messages.get(index);

    } else {
      return builder.getMessageOrBuilder();
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.setMessage() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public RepeatedFieldBuilderV3<MType, BType, IType> setMessage(int index, MType message) {
    checkNotNull(message);
    ensureMutableMessageList();
    messages.set(index, message);
    if (builders != null) {
      SingleFieldBuilderV3<MType, BType, IType> entry = builders.set(index, null);
      if (entry != null) {
        entry.dispose();
      }
    }
    onChanged();
    incrementModCounts();
    return this;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.addMessage() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public RepeatedFieldBuilderV3<MType, BType, IType> addMessage(MType message) {
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

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.addMessage() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public RepeatedFieldBuilderV3<MType, BType, IType> addMessage(int index, MType message) {
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

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.addAllMessages() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public RepeatedFieldBuilderV3<MType, BType, IType> addAllMessages(
      Iterable<? extends MType> values) {
    for (final MType value : values) {
      checkNotNull(value);
    }

    int size = -1;
    if (values instanceof Collection) {
      final Collection<?> collection = (Collection<?>) values;
      if (collection.isEmpty()) {
        return this;
      }
      size = collection.size();
    }
    ensureMutableMessageList();

    if (size >= 0 && messages instanceof ArrayList) {
      ((ArrayList<MType>) messages).ensureCapacity(messages.size() + size);
    }

    for (MType value : values) {
      addMessage(value);
    }

    onChanged();
    incrementModCounts();
    return this;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.addBuilder() instead.
   */
  @Deprecated
  public BType addBuilder(MType message) {
    ensureMutableMessageList();
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder =
        new SingleFieldBuilderV3<MType, BType, IType>(message, this, isClean);
    messages.add(null);
    builders.add(builder);
    onChanged();
    incrementModCounts();
    return builder.getBuilder();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.addBuilder() instead.
   */
  @Deprecated
  public BType addBuilder(int index, MType message) {
    ensureMutableMessageList();
    ensureBuilders();
    SingleFieldBuilderV3<MType, BType, IType> builder =
        new SingleFieldBuilderV3<MType, BType, IType>(message, this, isClean);
    messages.add(index, null);
    builders.add(index, builder);
    onChanged();
    incrementModCounts();
    return builder.getBuilder();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.remove() instead.
   */
  @Deprecated
  public void remove(int index) {
    ensureMutableMessageList();
    messages.remove(index);
    if (builders != null) {
      SingleFieldBuilderV3<MType, BType, IType> entry = builders.remove(index);
      if (entry != null) {
        entry.dispose();
      }
    }
    onChanged();
    incrementModCounts();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.clear() instead.
   */
  @Deprecated
  public void clear() {
    messages = Collections.emptyList();
    isMessagesListMutable = false;
    if (builders != null) {
      for (SingleFieldBuilderV3<MType, BType, IType> entry : builders) {
        if (entry != null) {
          entry.dispose();
        }
      }
      builders = null;
    }
    onChanged();
    incrementModCounts();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.build() instead.
   */
  @Deprecated
  public List<MType> build() {
    isClean = true;

    if (!isMessagesListMutable && builders == null) {
      return messages;
    }

    boolean allMessagesInSync = true;
    if (!isMessagesListMutable) {
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
        return messages;
      }
    }
    ensureMutableMessageList();
    for (int i = 0; i < messages.size(); i++) {
      messages.set(i, getMessage(i, true));
    }
    messages = Collections.unmodifiableList(messages);
    isMessagesListMutable = false;
    return messages;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getMessageList() instead.
   */
  @Deprecated
  public List<MType> getMessageList() {
    if (externalMessageList == null) {
      externalMessageList = new MessageExternalList<MType, BType, IType>(this);
    }
    return externalMessageList;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getBuilderList() instead.
   */
  @Deprecated
  public List<BType> getBuilderList() {
    if (externalBuilderList == null) {
      externalBuilderList = new BuilderExternalList<MType, BType, IType>(this);
    }
    return externalBuilderList;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.getMessageOrBuilderList() instead.
   */
  @Deprecated
  public List<IType> getMessageOrBuilderList() {
    if (externalMessageOrBuilderList == null) {
      externalMessageOrBuilderList = new MessageOrBuilderExternalList<MType, BType, IType>(this);
    }
    return externalMessageOrBuilderList;
  }

  private void onChanged() {
    if (isClean && parent != null) {
      parent.markDirty();
      isClean = false;
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * RepeatedFieldBuilder.markDirty() instead.
   */
  @Deprecated
  @Override
  public void markDirty() {
    onChanged();
  }

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

  private static class MessageExternalList<
          MType extends AbstractMessage,
          BType extends AbstractMessage.Builder,
          IType extends MessageOrBuilder>
      extends AbstractList<MType> implements List<MType>, RandomAccess {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    @Deprecated
    MessageExternalList(RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageExternalList.size() instead.
     */
    @Deprecated
    @Override
    public int size() {
      return this.builder.getCount();
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageExternalList.get() instead.
     */
    @Deprecated
    @Override
    public MType get(int index) {
      return builder.getMessage(index);
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageExternalList.incrementModCount() instead.
     */
    @Deprecated
    void incrementModCount() {
      modCount++;
    }
  }

  private static class BuilderExternalList<
          MType extends AbstractMessage,
          BType extends AbstractMessage.Builder,
          IType extends MessageOrBuilder>
      extends AbstractList<BType> implements List<BType>, RandomAccess {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    @Deprecated
    BuilderExternalList(RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.BuilderExternalList.size() instead.
     */
    @Deprecated
    @Override
    public int size() {
      return this.builder.getCount();
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.BuilderExternalList.get() instead.
     */
    @Deprecated
    @Override
    public BType get(int index) {
      return builder.getBuilder(index);
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.BuilderExternalList.incrementModCount() instead.
     */
    @Deprecated
    void incrementModCount() {
      modCount++;
    }
  }

  private static class MessageOrBuilderExternalList<
          MType extends AbstractMessage,
          BType extends AbstractMessage.Builder,
          IType extends MessageOrBuilder>
      extends AbstractList<IType> implements List<IType>, RandomAccess {

    RepeatedFieldBuilderV3<MType, BType, IType> builder;

    MessageOrBuilderExternalList(RepeatedFieldBuilderV3<MType, BType, IType> builder) {
      this.builder = builder;
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageOrBuilderExternalList.size() instead.
     */
    @Deprecated
    @Override
    public int size() {
      return this.builder.getCount();
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageOrBuilderExternalList.get() instead.
     */
    @Deprecated
    @Override
    public IType get(int index) {
      return builder.getMessageOrBuilder(index);
    }

    /*
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x). Users should update gencode to >= 4.26.x which uses
     * RepeatedFieldBuilder.MessageOrBuilderExternalList.incrementModCount() instead.
     */
    @Deprecated
    void incrementModCount() {
      modCount++;
    }
  }
}
