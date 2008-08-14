// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;
using System;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// TODO(jonskeet): Copy docs from Java
  /// </summary>
  public sealed class ExtensionRegistry {

    private static readonly ExtensionRegistry empty = new ExtensionRegistry(
        new Dictionary<string, ExtensionInfo>(),
        new Dictionary<DescriptorIntPair, ExtensionInfo>(),
        true);

    private readonly IDictionary<string, ExtensionInfo> extensionsByName;
    private readonly IDictionary<DescriptorIntPair, ExtensionInfo> extensionsByNumber;
    private readonly bool readOnly;

    private ExtensionRegistry(IDictionary<String, ExtensionInfo> extensionsByName,
        IDictionary<DescriptorIntPair, ExtensionInfo> extensionsByNumber,
        bool readOnly) {
      this.extensionsByName = extensionsByName;
      this.extensionsByNumber = extensionsByNumber;
      this.readOnly = readOnly;
    }

    /// <summary>
    /// Construct a new, empty instance.
    /// </summary>
    public static ExtensionRegistry CreateInstance() {
      return new ExtensionRegistry(new Dictionary<string, ExtensionInfo>(),
        new Dictionary<DescriptorIntPair, ExtensionInfo>(), false);
    }

    /// <summary>
    /// Get the unmodifiable singleton empty instance.
    /// </summary>
    public static ExtensionRegistry Empty {
      get { return empty; }
    }

    public ExtensionRegistry AsReadOnly() {
      return new ExtensionRegistry(extensionsByName, extensionsByNumber, true);
    }

    /// <summary>
    /// Finds an extension by fully-qualified field name, in the
    /// proto namespace, i.e. result.Descriptor.FullName will match
    /// <paramref name="fullName"/> if a match is found. A null
    /// reference is returned if the extension can't be found.
    /// </summary>
    public ExtensionInfo this[string fullName] {
      get {
        ExtensionInfo ret;
        extensionsByName.TryGetValue(fullName, out ret);
        return ret;
      }
    }

    /// <summary>
    /// Finds an extension by containing type and field number.
    /// A null reference is returned if the extension can't be found.
    /// </summary>
    public ExtensionInfo this[MessageDescriptor containingType, int fieldNumber] {
      get {
        ExtensionInfo ret;
        extensionsByNumber.TryGetValue(new DescriptorIntPair(containingType, fieldNumber), out ret);
        return ret;
      }
    }

    /// <summary>
    /// Add an extension from a generated file to the registry.
    /// </summary>
    public void Add<TContainer, TExtension> (GeneratedExtension<TContainer, TExtension> extension) 
        where TContainer : IMessage<TContainer> {
      if (extension.Descriptor.MappedType == MappedType.Message) {
        Add(new ExtensionInfo(extension.Descriptor, extension.MessageDefaultInstance));
      } else {
        Add(new ExtensionInfo(extension.Descriptor, null));
      }
    }

    /// <summary>
    /// Adds a non-message-type extension to the registry by descriptor.
    /// </summary>
    /// <param name="type"></param>
    public void Add(FieldDescriptor type) {
      if (type.MappedType == MappedType.Message) {
        throw new ArgumentException("ExtensionRegistry.Add() must be provided a default instance "
            + "when adding an embedded message extension.");
      }
      Add(new ExtensionInfo(type, null));
    }

    /// <summary>
    /// Adds a message-type-extension to the registry by descriptor.
    /// </summary>
    /// <param name="type"></param>
    /// <param name="defaultInstance"></param>
    public void Add(FieldDescriptor type, IMessage defaultInstance) {
      if (type.MappedType != MappedType.Message) {
        throw new ArgumentException("ExtensionRegistry.Add() provided a default instance for a "
            + "non-message extension.");
      }
      Add(new ExtensionInfo(type, defaultInstance));
    }

    private void Add(ExtensionInfo extension) {
      if (readOnly) {
        throw new InvalidOperationException("Cannot add entries to a read-only extension registry");
      }
      if (!extension.Descriptor.IsExtension) {
        throw new ArgumentException("ExtensionRegistry.add() was given a FieldDescriptor for a "
            + "regular (non-extension) field.");
      }

      extensionsByName[extension.Descriptor.FullName] = extension;
      extensionsByNumber[new DescriptorIntPair(extension.Descriptor.ContainingType,
          extension.Descriptor.FieldNumber)] = extension;

      FieldDescriptor field = extension.Descriptor;
      if (field.ContainingType.Options.IsMessageSetWireFormat
          && field.FieldType == FieldType.Message
          && field.IsOptional
          && field.ExtensionScope == field.MessageType) {
        // This is an extension of a MessageSet type defined within the extension
        // type's own scope.  For backwards-compatibility, allow it to be looked
        // up by type name.
        extensionsByName[field.MessageType.FullName] = extension;
      }
    }

    /// <summary>
    /// Nested type just used to represent a pair of MessageDescriptor and int, as
    /// the key into the "by number" map.
    /// </summary>
    private struct DescriptorIntPair : IEquatable<DescriptorIntPair> {
      readonly MessageDescriptor descriptor;
      readonly int number;

      internal DescriptorIntPair(MessageDescriptor descriptor, int number) {
        this.descriptor = descriptor;
        this.number = number;
      }

      public override int GetHashCode() {
        return descriptor.GetHashCode() * ((1 << 16) - 1) + number;
      }

      public override bool Equals(object obj) {
        if (!(obj is DescriptorIntPair)) {
          return false;
        }
        return Equals((DescriptorIntPair)obj);
      }

      public bool Equals(DescriptorIntPair other) {
        return descriptor == other.descriptor && number == other.number;
      }
    }
  }
}
