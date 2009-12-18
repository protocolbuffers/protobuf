// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * A table of known extensions, searchable by name or field number.  When
 * parsing a protocol message that might have extensions, you must provide
 * an {@code ExtensionRegistry} in which you have registered any extensions
 * that you want to be able to parse.  Otherwise, those extensions will just
 * be treated like unknown fields.
 *
 * <p>For example, if you had the {@code .proto} file:
 *
 * <pre>
 * option java_class = "MyProto";
 *
 * message Foo {
 *   extensions 1000 to max;
 * }
 *
 * extend Foo {
 *   optional int32 bar;
 * }
 * </pre>
 *
 * Then you might write code like:
 *
 * <pre>
 * ExtensionRegistry registry = ExtensionRegistry.newInstance();
 * registry.add(MyProto.bar);
 * MyProto.Foo message = MyProto.Foo.parseFrom(input, registry);
 * </pre>
 *
 * <p>Background:
 *
 * <p>You might wonder why this is necessary.  Two alternatives might come to
 * mind.  First, you might imagine a system where generated extensions are
 * automatically registered when their containing classes are loaded.  This
 * is a popular technique, but is bad design; among other things, it creates a
 * situation where behavior can change depending on what classes happen to be
 * loaded.  It also introduces a security vulnerability, because an
 * unprivileged class could cause its code to be called unexpectedly from a
 * privileged class by registering itself as an extension of the right type.
 *
 * <p>Another option you might consider is lazy parsing: do not parse an
 * extension until it is first requested, at which point the caller must
 * provide a type to use.  This introduces a different set of problems.  First,
 * it would require a mutex lock any time an extension was accessed, which
 * would be slow.  Second, corrupt data would not be detected until first
 * access, at which point it would be much harder to deal with it.  Third, it
 * could violate the expectation that message objects are immutable, since the
 * type provided could be any arbitrary message class.  An unprivileged user
 * could take advantage of this to inject a mutable object into a message
 * belonging to privileged code and create mischief.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class ExtensionRegistry extends ExtensionRegistryLite {
  /** Construct a new, empty instance. */
  public static ExtensionRegistry newInstance() {
    return new ExtensionRegistry();
  }

  /** Get the unmodifiable singleton empty instance. */
  public static ExtensionRegistry getEmptyRegistry() {
    return EMPTY;
  }

  /** Returns an unmodifiable view of the registry. */
  @Override
  public ExtensionRegistry getUnmodifiable() {
    return new ExtensionRegistry(this);
  }

  /** A (Descriptor, Message) pair, returned by lookup methods. */
  public static final class ExtensionInfo {
    /** The extension's descriptor. */
    public final FieldDescriptor descriptor;

    /**
     * A default instance of the extension's type, if it has a message type.
     * Otherwise, {@code null}.
     */
    public final Message defaultInstance;

    private ExtensionInfo(final FieldDescriptor descriptor) {
      this.descriptor = descriptor;
      defaultInstance = null;
    }
    private ExtensionInfo(final FieldDescriptor descriptor,
                          final Message defaultInstance) {
      this.descriptor = descriptor;
      this.defaultInstance = defaultInstance;
    }
  }

  /**
   * Find an extension by fully-qualified field name, in the proto namespace.
   * I.e. {@code result.descriptor.fullName()} will match {@code fullName} if
   * a match is found.
   *
   * @return Information about the extension if found, or {@code null}
   *         otherwise.
   */
  public ExtensionInfo findExtensionByName(final String fullName) {
    return extensionsByName.get(fullName);
  }

  /**
   * Find an extension by containing type and field number.
   *
   * @return Information about the extension if found, or {@code null}
   *         otherwise.
   */
  public ExtensionInfo findExtensionByNumber(final Descriptor containingType,
                                             final int fieldNumber) {
    return extensionsByNumber.get(
      new DescriptorIntPair(containingType, fieldNumber));
  }

  /** Add an extension from a generated file to the registry. */
  public void add(final GeneratedMessage.GeneratedExtension<?, ?> extension) {
    if (extension.getDescriptor().getJavaType() ==
        FieldDescriptor.JavaType.MESSAGE) {
      if (extension.getMessageDefaultInstance() == null) {
        throw new IllegalStateException(
            "Registered message-type extension had null default instance: " +
            extension.getDescriptor().getFullName());
      }
      add(new ExtensionInfo(extension.getDescriptor(),
                            extension.getMessageDefaultInstance()));
    } else {
      add(new ExtensionInfo(extension.getDescriptor(), null));
    }
  }

  /** Add a non-message-type extension to the registry by descriptor. */
  public void add(final FieldDescriptor type) {
    if (type.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
      throw new IllegalArgumentException(
        "ExtensionRegistry.add() must be provided a default instance when " +
        "adding an embedded message extension.");
    }
    add(new ExtensionInfo(type, null));
  }

  /** Add a message-type extension to the registry by descriptor. */
  public void add(final FieldDescriptor type, final Message defaultInstance) {
    if (type.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
      throw new IllegalArgumentException(
        "ExtensionRegistry.add() provided a default instance for a " +
        "non-message extension.");
    }
    add(new ExtensionInfo(type, defaultInstance));
  }

  // =================================================================
  // Private stuff.

  private ExtensionRegistry() {
    this.extensionsByName = new HashMap<String, ExtensionInfo>();
    this.extensionsByNumber = new HashMap<DescriptorIntPair, ExtensionInfo>();
  }

  private ExtensionRegistry(ExtensionRegistry other) {
    super(other);
    this.extensionsByName = Collections.unmodifiableMap(other.extensionsByName);
    this.extensionsByNumber =
        Collections.unmodifiableMap(other.extensionsByNumber);
  }

  private final Map<String, ExtensionInfo> extensionsByName;
  private final Map<DescriptorIntPair, ExtensionInfo> extensionsByNumber;

  private ExtensionRegistry(boolean empty) {
    super(ExtensionRegistryLite.getEmptyRegistry());
    this.extensionsByName = Collections.<String, ExtensionInfo>emptyMap();
    this.extensionsByNumber =
        Collections.<DescriptorIntPair, ExtensionInfo>emptyMap();
  }
  private static final ExtensionRegistry EMPTY = new ExtensionRegistry(true);

  private void add(final ExtensionInfo extension) {
    if (!extension.descriptor.isExtension()) {
      throw new IllegalArgumentException(
        "ExtensionRegistry.add() was given a FieldDescriptor for a regular " +
        "(non-extension) field.");
    }

    extensionsByName.put(extension.descriptor.getFullName(), extension);
    extensionsByNumber.put(
      new DescriptorIntPair(extension.descriptor.getContainingType(),
                            extension.descriptor.getNumber()),
      extension);

    final FieldDescriptor field = extension.descriptor;
    if (field.getContainingType().getOptions().getMessageSetWireFormat() &&
        field.getType() == FieldDescriptor.Type.MESSAGE &&
        field.isOptional() &&
        field.getExtensionScope() == field.getMessageType()) {
      // This is an extension of a MessageSet type defined within the extension
      // type's own scope.  For backwards-compatibility, allow it to be looked
      // up by type name.
      extensionsByName.put(field.getMessageType().getFullName(), extension);
    }
  }

  /** A (GenericDescriptor, int) pair, used as a map key. */
  private static final class DescriptorIntPair {
    private final Descriptor descriptor;
    private final int number;

    DescriptorIntPair(final Descriptor descriptor, final int number) {
      this.descriptor = descriptor;
      this.number = number;
    }

    @Override
    public int hashCode() {
      return descriptor.hashCode() * ((1 << 16) - 1) + number;
    }
    @Override
    public boolean equals(final Object obj) {
      if (!(obj instanceof DescriptorIntPair)) {
        return false;
      }
      final DescriptorIntPair other = (DescriptorIntPair)obj;
      return descriptor == other.descriptor && number == other.number;
    }
  }
}
