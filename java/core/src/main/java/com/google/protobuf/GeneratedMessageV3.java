// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.List;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Extension.ExtensionType;
import com.google.protobuf.GeneratedMessage.ExtendableMessageOrBuilder;
import com.google.protobuf.GeneratedMessage.ExtensionDescriptorRetriever;
import com.google.protobuf.GeneratedMessage.FieldAccessorTable;
/**
 *  Stub for GeneratedMessageV3 wrapping GeneratedMessage for compatibility with older gencode.
 *  This class is deprecated, and slated for removal in the next Java breaking change (5.x in 2025 Q1).
 *  Users should update gencode to >= 4.26.x which uses GeneratedMessage instead of GeneratedMessageV3.
 */
@Deprecated
public abstract class GeneratedMessageV3 extends GeneratedMessage {
  public static boolean alwaysUseFieldBuilders = GeneratedMessage.alwaysUseFieldBuilders;

  protected GeneratedMessageV3() {
    super();
  }

  protected GeneratedMessageV3(Builder<?> builder) {
    super(builder);
  }

  @Deprecated
  public abstract static class ExtendableBuilder<
          MessageT extends ExtendableMessage<MessageT>,
          BuilderT extends ExtendableBuilder<MessageT, BuilderT>>
      extends GeneratedMessage.ExtendableBuilder<MessageT, BuilderT> implements ExtendableMessageOrBuilder<MessageT> {
    protected ExtendableBuilder() {
      super();
    }

    protected ExtendableBuilder(BuilderParent parent) {
      super(parent);
    }

    // Support old gencode override method removed in cl/597677225
    public <T> BuilderT setExtension(
        final GeneratedMessage.GeneratedExtension<MessageT, T> extension, final T value) {
      return setExtension((ExtensionLite<MessageT, T>) extension, value);
    }

    // Support old gencode override method removed in cl/597677225
    public <T> BuilderT setExtension(
        final GeneratedMessage.GeneratedExtension<MessageT, List<T>> extension,
        final int index, final T value) {
      return setExtension((ExtensionLite<MessageT, List<T>>) extension, index, value);
    }

    // Support old gencode override method removed in cl/597677225
    public <T> BuilderT addExtension(
        final GeneratedMessage.GeneratedExtension<MessageT, List<T>> extension, final T value) {
      return addExtension((ExtensionLite<MessageT, List<T>>) extension, value);
    }

    // Support old gencode override method removed in cl/597677225
    public <T> BuilderT clearExtension(
        final GeneratedMessage.GeneratedExtension<MessageT, T> extension) {
      return clearExtension((ExtensionLite<MessageT, T>) extension);
    }
  }
}