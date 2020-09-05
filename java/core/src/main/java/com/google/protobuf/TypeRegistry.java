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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;

/**
 * A TypeRegistry is used to resolve Any messages. You must provide a TypeRegistry containing all
 * message types used in Any message fields.
 */
public class TypeRegistry {
  private static final Logger logger = Logger.getLogger(TypeRegistry.class.getName());

  private static class EmptyTypeRegistryHolder {
    private static final TypeRegistry EMPTY =
        new TypeRegistry(Collections.<String, Descriptor>emptyMap());
  }

  public static TypeRegistry getEmptyTypeRegistry() {
    return EmptyTypeRegistryHolder.EMPTY;
  }


  public static Builder newBuilder() {
    return new Builder();
  }

  /**
   * Find a type by its full name. Returns null if it cannot be found in this {@link TypeRegistry}.
   */
  public Descriptor find(String name) {
    return types.get(name);
  }

  /**
   * Find a type by its typeUrl. Returns null if it cannot be found in this {@link TypeRegistry}.
   */
  /* @Nullable */
  public final Descriptor getDescriptorForTypeUrl(String typeUrl)
      throws InvalidProtocolBufferException {
    return find(getTypeName(typeUrl));
  }

  private final Map<String, Descriptor> types;

  TypeRegistry(Map<String, Descriptor> types) {
    this.types = types;
  }

  private static String getTypeName(String typeUrl) throws InvalidProtocolBufferException {
    String[] parts = typeUrl.split("/");
    if (parts.length == 1) {
      throw new InvalidProtocolBufferException("Invalid type url found: " + typeUrl);
    }
    return parts[parts.length - 1];
  }

  /** A Builder is used to build {@link TypeRegistry}. */
  public static final class Builder {
    private Builder() {}

    /**
     * Adds a message type and all types defined in the same .proto file as well as all transitively
     * imported .proto files to this {@link Builder}.
     */
    public Builder add(Descriptor messageType) {
      if (types == null) {
        throw new IllegalStateException("A TypeRegistry.Builder can only be used once.");
      }
      addFile(messageType.getFile());
      return this;
    }

    /**
     * Adds message types and all types defined in the same .proto file as well as all transitively
     * imported .proto files to this {@link Builder}.
     */
    public Builder add(Iterable<Descriptor> messageTypes) {
      if (types == null) {
        throw new IllegalStateException("A TypeRegistry.Builder can only be used once.");
      }
      for (Descriptor type : messageTypes) {
        addFile(type.getFile());
      }
      return this;
    }

    /** Builds a {@link TypeRegistry}. This method can only be called once for one Builder. */
    public TypeRegistry build() {
      TypeRegistry result = new TypeRegistry(types);
      // Make sure the built {@link TypeRegistry} is immutable.
      types = null;
      return result;
    }

    private void addFile(FileDescriptor file) {
      // Skip the file if it's already added.
      if (!files.add(file.getFullName())) {
        return;
      }
      for (FileDescriptor dependency : file.getDependencies()) {
        addFile(dependency);
      }
      for (Descriptor message : file.getMessageTypes()) {
        addMessage(message);
      }
    }

    private void addMessage(Descriptor message) {
      for (Descriptor nestedType : message.getNestedTypes()) {
        addMessage(nestedType);
      }

      if (types.containsKey(message.getFullName())) {
        logger.warning("Type " + message.getFullName() + " is added multiple times.");
        return;
      }

      types.put(message.getFullName(), message);
    }

    private final Set<String> files = new HashSet<>();
    private Map<String, Descriptor> types = new HashMap<>();
  }
}
