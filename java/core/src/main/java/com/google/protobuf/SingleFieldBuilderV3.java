// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Stub for SingleFieldBuilderV3 wrapping SingleFieldBuilder for compatibility with older gencode.
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
    extends SingleFieldBuilder<MType, BType, IType> {

  public SingleFieldBuilderV3(
    MType message, AbstractMessage.BuilderParent parent, boolean isClean) {
    super(message, parent, isClean);
  }

  /* Returns SingleFieldBuilderV3 instead of SingleFieldBuilder.
  *
  * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
  * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
  * SingleFieldBuilder.setMessage instead of SingleFieldBuilderV3.setMessage.
  */
  @Deprecated
  @Override
  public SingleFieldBuilderV3<MType, BType, IType> setMessage(MType message) {
    super.setMessage(message);
    return this;
  }

  /* Returns SingleFieldBuilderV3 instead of SingleFieldBuilder.
  *
  * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
  * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
  * SingleFieldBuilder.setMessage instead of SingleFieldBuilderV3.setMessage.
  */
  @Deprecated
  @Override
  public SingleFieldBuilderV3<MType, BType, IType> mergeFrom(MType value) {
    super.mergeFrom(value);
    return this;
  }

  /* Returns SingleFieldBuilderV3 instead of SingleFieldBuilder.
  *
  * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
  * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
  * SingleFieldBuilder.setMessage instead of SingleFieldBuilderV3.setMessage.
  */
  @Deprecated
  @Override
  public SingleFieldBuilderV3<MType, BType, IType> clear() {
    super.clear();
    return this;
  }
}
