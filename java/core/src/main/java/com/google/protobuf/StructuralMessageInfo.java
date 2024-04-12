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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Information for the layout of a protobuf message class. This describes all of the fields
 * contained within a message.
 */
@ExperimentalApi
@CheckReturnValue
final class StructuralMessageInfo implements MessageInfo {
  private final ProtoSyntax syntax;
  private final boolean messageSetWireFormat;
  private final int[] checkInitialized;
  private final FieldInfo[] fields;
  private final MessageLite defaultInstance;

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
    this.syntax = syntax;
    this.messageSetWireFormat = messageSetWireFormat;
    this.checkInitialized = checkInitialized;
    this.fields = fields;
    this.defaultInstance = (MessageLite) checkNotNull(defaultInstance, "defaultInstance");
  }

  /** Gets the syntax for the message (e.g. PROTO2, PROTO3). */
  @Override
  public ProtoSyntax getSyntax() {
    return syntax;
  }

  /** Indicates whether or not the message should be represented with message set wire format. */
  @Override
  public boolean isMessageSetWireFormat() {
    return messageSetWireFormat;
  }

  /** An array of field numbers that need to be checked for isInitialized(). */
  public int[] getCheckInitialized() {
    return checkInitialized;
  }

  /**
   * Gets the information for all fields within this message, sorted in ascending order by their
   * field number.
   */
  public FieldInfo[] getFields() {
    return fields;
  }

  @Override
  public MessageLite getDefaultInstance() {
    return defaultInstance;
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
    private ProtoSyntax syntax;
    private boolean wasBuilt;
    private boolean messageSetWireFormat;
    private int[] checkInitialized = null;
    private Object defaultInstance;

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
