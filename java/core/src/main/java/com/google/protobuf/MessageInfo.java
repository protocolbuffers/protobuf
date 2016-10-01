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
public final class MessageInfo {
  private final ProtoSyntax syntax;
  private final boolean messageSetWireFormat;
  private final List<FieldInfo> fields;

  /**
   * Constructor.
   *
   * @param fields the set of fields for the message.
   */
  private MessageInfo(ProtoSyntax syntax, boolean messageSetWireFormat, List<FieldInfo> fields) {
    this.syntax = syntax;
    this.messageSetWireFormat = messageSetWireFormat;
    this.fields = fields;
  }

  /** Gets the syntax for the message (e.g. PROTO2, PROTO3). */
  public ProtoSyntax getSyntax() {
    return syntax;
  }

  /** Indicates whether or not the message should be represented with message set wire format. */
  public boolean isMessageSetWireFormat() {
    return messageSetWireFormat;
  }

  /**
   * Gets the information for all fields within this message, sorted in ascending order by their
   * field number.
   */
  public List<FieldInfo> getFields() {
    return fields;
  }

  /** Creates a new map of field number to message class for message fields. */
  public Int2ObjectHashMap<Class<?>> messageFieldClassMap() {
    Int2ObjectHashMap<Class<?>> classMap = new Int2ObjectHashMap<Class<?>>();
    for (int i = 0; i < fields.size(); ++i) {
      FieldInfo fd = fields.get(i);
      int fieldNumber = fd.getFieldNumber();

      // Configure messages
      switch (fd.getType()) {
        case MESSAGE:
          classMap.put(fieldNumber, fd.getField().getType());
          break;
        case MESSAGE_LIST:
          classMap.put(fieldNumber, fd.getListElementType());
          break;
        case GROUP:
          classMap.put(fieldNumber, fd.getField().getType());
          break;
        case GROUP_LIST:
          classMap.put(fieldNumber, fd.getListElementType());
          break;
        default:
          break;
      }
    }
    return classMap;
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
    private final ArrayList<FieldInfo> fields;
    private ProtoSyntax syntax;
    private boolean wasBuilt;
    private boolean messageSetWireFormat;

    public Builder() {
      fields = new ArrayList<FieldInfo>();
    }

    public Builder(int numFields) {
      fields = new ArrayList<FieldInfo>(numFields);
    }

    public void withSyntax(ProtoSyntax syntax) {
      this.syntax = checkNotNull(syntax, "syntax");
    }

    public void withMessageSetWireFormat(boolean messageSetWireFormat) {
      this.messageSetWireFormat = messageSetWireFormat;
    }

    public void add(FieldInfo field) {
      if (wasBuilt) {
        throw new IllegalStateException("Builder can only build once");
      }
      fields.add(field);
    }

    public MessageInfo build() {
      if (wasBuilt) {
        throw new IllegalStateException("Builder can only build once");
      }
      if (syntax == null) {
        throw new IllegalStateException("Must specify a proto syntax");
      }
      wasBuilt = true;
      Collections.sort(fields);
      return new MessageInfo(syntax, messageSetWireFormat, Collections.unmodifiableList(fields));
    }
  }
}
