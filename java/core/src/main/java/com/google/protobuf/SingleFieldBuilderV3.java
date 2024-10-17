// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

/**
 * Stub for SingleFieldBuilderV3 matching SingleFieldBuilder for compatibility with older gencode.
 * This cannot wrap SingleFieldBuilder directly due to SingleFieldBuilder having more restrictive
 * extends GeneratedMessage for MType and BType.
 *
 * @deprecated This class is deprecated, and slated for removal in the next breaking change. Users
 *     should update gencode to >= 4.26.x which replaces SingleFieldBuilderV3 with
 *     SingleFieldBuilder.
 */
@Deprecated
public class SingleFieldBuilderV3<
        MType extends AbstractMessage,
        BType extends AbstractMessage.Builder,
        IType extends MessageOrBuilder>
    implements AbstractMessage.BuilderParent {

  private AbstractMessage.BuilderParent parent;

  private BType builder;

  private MType message;

  private boolean isClean;

  @Deprecated
  public SingleFieldBuilderV3(
      MType message, AbstractMessage.BuilderParent parent, boolean isClean) {
    this.message = checkNotNull(message);
    this.parent = parent;
    this.isClean = isClean;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.dispose() instead.
   */
  @Deprecated
  public void dispose() {
    parent = null;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.getMessage() instead.
   */
  @Deprecated
  @SuppressWarnings("unchecked")
  public MType getMessage() {
    if (message == null) {
      message = (MType) builder.buildPartial();
    }
    return message;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.build() instead.
   */
  @Deprecated
  public MType build() {
    isClean = true;
    return getMessage();
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.getbuilder() instead.
   */
  @Deprecated
  @SuppressWarnings("unchecked")
  public BType getBuilder() {
    if (builder == null) {
      builder = (BType) message.newBuilderForType(this);
      builder.mergeFrom(message);
      builder.markClean();
    }
    return builder;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.getMessageOrBuilder() instead.
   */
  @Deprecated
  @SuppressWarnings("unchecked")
  public IType getMessageOrBuilder() {
    if (builder != null) {
      return (IType) builder;
    } else {
      return (IType) message;
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.setMessage() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public SingleFieldBuilderV3<MType, BType, IType> setMessage(MType message) {
    this.message = checkNotNull(message);
    if (builder != null) {
      builder.dispose();
      builder = null;
    }
    onChanged();
    return this;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.mergeFrom() instead.
   */
  @Deprecated
  @CanIgnoreReturnValue
  public SingleFieldBuilderV3<MType, BType, IType> mergeFrom(MType value) {
    if (builder == null && message == message.getDefaultInstanceForType()) {
      message = value;
    } else {
      getBuilder().mergeFrom(value);
    }
    onChanged();
    return this;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.clear() instead.
   */
  @Deprecated
  @SuppressWarnings("unchecked")
  @CanIgnoreReturnValue
  public SingleFieldBuilderV3<MType, BType, IType> clear() {
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
    isClean = true;
    return this;
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.onChanged() instead.
   */
  @Deprecated
  private void onChanged() {
    if (builder != null) {
      message = null;
    }
    if (isClean && parent != null) {
      parent.markDirty();

      isClean = false;
    }
  }

  /*
   * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
   * (5.x). Users should update gencode to >= 4.26.x which uses
   * SingleFieldBuilder.markDirty() instead.
   */
  @Deprecated
  @Override
  public void markDirty() {
    onChanged();
  }
}
