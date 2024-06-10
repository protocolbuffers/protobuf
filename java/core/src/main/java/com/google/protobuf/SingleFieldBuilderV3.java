// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

/**
 *  Stub for RepeatedFieldBuilderV3 wrapping RepeatedFieldBuilder for compatibility with older gencode.
 *  This class is deprecated, and slated for removal in the next breaking change.
 *  Users should update gencode to >= 4.26.x which replaces RepeatedFieldBuilderV3 with RepeatedFieldBuilder.
 */
@Deprecated
public class SingleFieldBuilderV3<
        MType extends GeneratedMessage,
        BType extends GeneratedMessage.Builder,
        IType extends MessageOrBuilder>
    extends SingleFieldBuilder<MType, BType, IType> {
  
  public SingleFieldBuilderV3(MType message, GeneratedMessage.BuilderParent parent, boolean isClean) {
    super(message, parent, isClean);
  }
}