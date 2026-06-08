// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import javax.annotation.Nullable;

/**
 * Information for the layout of a protobuf message class. This describes all of the fields
 * contained within a message.
 */
@SuppressWarnings("nullness")
@ExperimentalApi
@CheckReturnValue
final class StructuralMessageInfo implements MessageInfo {
  private final ProtoSyntax syntax_;
  private final boolean messageSetWireFormat;
  private final int[] checkInitialized_;
  private final FieldInfo[] fields_;
  private final MessageLite defaultInstance_;

  /**
   * Constructor.
   *
   * @param checkInitialized fields to check in isInitialized().
   * @param fields the set of fields for the message, in field number order.
   */
  StructuralMessageInfo(
      ProtoSyntax syntax,
      boolean messageSetWireFormat,
      int[] checkInitialized,
      FieldInfo[] fields,
      Object defaultInstance) {
    this.syntax_ = syntax;
    this.messageSetWireFormat = messageSetWireFormat;
    this.checkInitialized_ = checkInitialized;
    this.fields_ = fields;
    this.defaultInstance_ = (MessageLite) checkNotNull(defaultInstance, "defaultInstance");
  }

  /** Gets the syntax for the message (e.g. PROTO2, PROTO3). */
  @Override
  public ProtoSyntax getSyntax() {
    return syntax_;
  }

  /** Indicates whether or not the message should be represented with message set wire format. */
  @Override
  public boolean isMessageSetWireFormat() {
    return messageSetWireFormat;
  }

  /** An array of field numbers that need to be checked for isInitialized(). */
  public int[] getCheckInitialized() {
    return checkInitialized_;
  }

  /**
   * Gets the information for all fields within this message, sorted in ascending order by their
   * field number.
   */
  public FieldInfo[] getFields() {
    return fields_;
  }

  @Override
  public MessageLite getDefaultInstance() {
    return defaultInstance_;
  }

  /** Helper method for creating a new builder for {@link MessageInfo}. */
  public static Builder newBuilder() {
    return new Builder();
  }

  /** Helper method for creating a new builder for {@link MessageInfo}. */
  public static Builder newBuilder(int numFields) {
    return new Builder(numFields);
  }

  /** A builder of {@link MessageInfo} instances. */
  public static final class Builder {
    private final List<FieldInfo> fields;
    private @Nullable ProtoSyntax syntax;
    private boolean wasBuilt;
    private boolean messageSetWireFormat;
    private int[] checkInitialized = null;
    private @Nullable Object defaultInstance;

    public Builder() {
      fields = new ArrayList<FieldInfo>();
    }

    public Builder(int numFields) {
      fields = new ArrayList<FieldInfo>(numFields);
    }

    public void withDefaultInstance(Object defaultInstance) {
      this.defaultInstance = defaultInstance;
    }

    public void withSyntax(ProtoSyntax syntax) {
      this.syntax = checkNotNull(syntax, "syntax");
    }

    public void withMessageSetWireFormat(boolean messageSetWireFormat) {
      this.messageSetWireFormat = messageSetWireFormat;
    }

    public void withCheckInitialized(int[] checkInitialized) {
      this.checkInitialized = checkInitialized;
    }

    public void withField(FieldInfo field) {
      if (wasBuilt) {
        throw new IllegalStateException("Builder can only build once");
      }
      fields.add(field);
    }

    public StructuralMessageInfo build() {
      if (wasBuilt) {
        throw new IllegalStateException("Builder can only build once");
      }
      if (syntax == null) {
        throw new IllegalStateException("Must specify a proto syntax");
      }
      wasBuilt = true;
      Collections.sort(fields);
      return new StructuralMessageInfo(
          syntax,
          messageSetWireFormat,
          checkInitialized,
          fields.toArray(new FieldInfo[0]),
          defaultInstance);
    }
  }
}
