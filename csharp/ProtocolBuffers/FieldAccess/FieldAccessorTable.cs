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
using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Provides access to fields in generated messages via reflection.
  /// This type is public to allow it to be used by generated messages, which
  /// create appropriate instances in the .proto file description class.
  /// TODO(jonskeet): See if we can hide it somewhere...
  /// </summary>
  public sealed class FieldAccessorTable<TMessage, TBuilder>
      where TMessage : IMessage<TMessage, TBuilder>
      where TBuilder : IBuilder<TMessage, TBuilder> {

    readonly IFieldAccessor<TMessage, TBuilder>[] accessors;

    readonly MessageDescriptor descriptor;

    public MessageDescriptor Descriptor { 
      get { return descriptor; }
    }

   /// <summary>
    /// Constructs a FieldAccessorTable for a particular message class.
    /// Only one FieldAccessorTable should be constructed per class.
    /// </summary>
    /// <param name="descriptor">The type's descriptor</param>
    /// <param name="propertyNames">The Pascal-case names of all the field-based properties in the message.</param>
    public FieldAccessorTable(MessageDescriptor descriptor, String[] propertyNames) {
      this.descriptor = descriptor;
      accessors = new IFieldAccessor<TMessage, TBuilder>[descriptor.Fields.Count];
      for (int i=0; i < accessors.Length; i++) {
        accessors[i] = CreateAccessor(descriptor.Fields[i], propertyNames[i]);
      }
    }

     /// <summary>
     /// Creates an accessor for a single field
     /// </summary>   
    private static IFieldAccessor<TMessage, TBuilder> CreateAccessor(FieldDescriptor field, string name) {
      if (field.IsRepeated) {
        switch (field.MappedType) {
          case MappedType.Message: return new RepeatedMessageAccessor<TMessage, TBuilder>(name);
          case MappedType.Enum: return new RepeatedEnumAccessor<TMessage, TBuilder>(field, name);
          default: return new RepeatedPrimitiveAccessor<TMessage, TBuilder>(name);
        }
      } else {
        switch (field.MappedType) {
          case MappedType.Message: return new SingleMessageAccessor<TMessage, TBuilder>(name);
          case MappedType.Enum: return new SingleEnumAccessor<TMessage, TBuilder>(field, name);
          default: return new SinglePrimitiveAccessor<TMessage, TBuilder>(name);
        }
      }
    }

    internal IFieldAccessor<TMessage, TBuilder> this[FieldDescriptor field] {
      get {
        if (field.ContainingType != descriptor) {
          throw new ArgumentException("FieldDescriptor does not match message type.");
        } else if (field.IsExtension) {
          // If this type had extensions, it would subclass ExtendableMessage,
          // which overrides the reflection interface to handle extensions.
          throw new ArgumentException("This type does not have extensions.");
        }
        return accessors[field.Index];
      }
    }
  }
}
