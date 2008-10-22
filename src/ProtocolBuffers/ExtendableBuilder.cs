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

namespace Google.ProtocolBuffers {
  public abstract class ExtendableBuilder<TMessage, TBuilder> : GeneratedBuilder<TMessage, TBuilder>
    where TMessage : ExtendableMessage<TMessage, TBuilder>
    where TBuilder : GeneratedBuilder<TMessage, TBuilder> {

    protected ExtendableBuilder() {}

    /// <summary>
    /// Checks if a singular extension is present
    /// </summary>
    public bool HasExtension<TExtension>(GeneratedExtensionBase<TExtension> extension) {
      return MessageBeingBuilt.HasExtension(extension);
    }

    /// <summary>
    /// Returns the number of elements in a repeated extension.
    /// </summary>
    public int GetExtensionCount<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension) {
      return MessageBeingBuilt.GetExtensionCount(extension);
    }

    /// <summary>
    /// Returns the value of an extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TExtension> extension) {
      return MessageBeingBuilt.GetExtension(extension);
    }

    /// <summary>
    /// Returns one element of a repeated extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension, int index) {
      return MessageBeingBuilt.GetExtension(extension, index);
    }

    /// <summary>
    /// Sets the value of an extension.
    /// </summary>
    public TBuilder SetExtension<TExtension>(GeneratedExtensionBase<TExtension> extension, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor] = extension.ToReflectionType(value);
      return ThisBuilder;
    }

    /// <summary>
    /// Sets the value of one element of a repeated extension.
    /// </summary>
    public TBuilder SetExtension<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension, int index, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor, index] = extension.SingularToReflectionType(value);
      return ThisBuilder;
    }

    /// <summary>
    /// Appends a value to a repeated extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> AddExtension<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions.AddRepeatedField(extension.Descriptor, extension.SingularToReflectionType(value));
      return this;
    }

    /// <summary>
    /// Clears an extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> ClearExtension<TExtension>(GeneratedExtensionBase<TExtension> extension) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions.ClearField(extension.Descriptor);
      return this;
    }

    /// <summary>
    /// Called by subclasses to parse an unknown field or an extension.
    /// </summary>
    /// <returns>true unless the tag is an end-group tag</returns>
    protected override bool ParseUnknownField(CodedInputStream input, UnknownFieldSet.Builder unknownFields,
        ExtensionRegistry extensionRegistry, uint tag) {
      return unknownFields.MergeFieldFrom(input, extensionRegistry, this, tag);
    }

    // ---------------------------------------------------------------
    // Reflection


    public override object this[FieldDescriptor field, int index] {
      set {
        if (field.IsExtension) {
          ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
          message.VerifyContainingType(field);
          message.Extensions[field, index] = value;
        } else {
          base[field, index] = value;
        }
      }
    }

    
    public override object this[FieldDescriptor field] {
      set {
        if (field.IsExtension) {
          ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
          message.VerifyContainingType(field);
          message.Extensions[field] = value;
        } else {
          base[field] = value;
        }
      }
    }

    public override TBuilder ClearField(FieldDescriptor field) {
      if (field.IsExtension) {
        ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
        message.VerifyContainingType(field);
        message.Extensions.ClearField(field);
        return ThisBuilder;
      } else {
        return base.ClearField(field);
      }
    }

    public override TBuilder AddRepeatedField(FieldDescriptor field, object value) {
      if (field.IsExtension) {
        ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
        message.VerifyContainingType(field); 
        message.Extensions.AddRepeatedField(field, value);
        return ThisBuilder;
      } else {
        return base.AddRepeatedField(field, value);
      }
    }
  }
}
