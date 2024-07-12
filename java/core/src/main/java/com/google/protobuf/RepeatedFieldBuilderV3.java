// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.List;

/**
 * Stub for RepeatedFieldBuilderV3 wrapping RepeatedFieldBuilder for compatibility with older
 * gencode.
 *
 * @deprecated This class is deprecated, and slated for removal in the next breaking change. Users
 *     should update gencode to >= 4.26.x which replaces RepeatedFieldBuilderV3 with
 *     RepeatedFieldBuilder.
 */
@Deprecated
public class RepeatedFieldBuilderV3<
        MType extends GeneratedMessage,
        BType extends GeneratedMessage.Builder,
        IType extends MessageOrBuilder>
    extends RepeatedFieldBuilder<MType, BType, IType> {

  public RepeatedFieldBuilderV3(
      List<MType> messages,
      boolean isMessagesListMutable,
      GeneratedMessage.BuilderParent parent,
      boolean isClean) {
    super(messages, isMessagesListMutable, parent, isClean);
  }
}
