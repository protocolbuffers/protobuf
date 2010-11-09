#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
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
#endregion

using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  public abstract class ExtendableBuilderLite<TMessage, TBuilder> : GeneratedBuilderLite<TMessage, TBuilder>
    where TMessage : ExtendableMessageLite<TMessage, TBuilder>
    where TBuilder : GeneratedBuilderLite<TMessage, TBuilder> {

    protected ExtendableBuilderLite() { }

    /// <summary>
    /// Checks if a singular extension is present
    /// </summary>
    public bool HasExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension) {
      return MessageBeingBuilt.HasExtension(extension);
    }

    /// <summary>
    /// Returns the number of elements in a repeated extension.
    /// </summary>
    public int GetExtensionCount<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension) {
      return MessageBeingBuilt.GetExtensionCount(extension);
    }

    /// <summary>
    /// Returns the value of an extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension) {
      return MessageBeingBuilt.GetExtension(extension);
    }

    /// <summary>
    /// Returns one element of a repeated extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension, int index) {
      return MessageBeingBuilt.GetExtension(extension, index);
    }

    /// <summary>
    /// Sets the value of an extension.
    /// </summary>
    public TBuilder SetExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension, TExtension value) {
      ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor] = extension.ToReflectionType(value);
      return ThisBuilder;
    }

    /// <summary>
    /// Sets the value of one element of a repeated extension.
    /// </summary>
    public TBuilder SetExtension<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension, int index, TExtension value) {
      ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor, index] = extension.SingularToReflectionType(value);
      return ThisBuilder;
    }

    /// <summary>
    /// Appends a value to a repeated extension.
    /// </summary>
    public TBuilder AddExtension<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension, TExtension value) {
      ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions.AddRepeatedField(extension.Descriptor, extension.SingularToReflectionType(value));
      return ThisBuilder;
    }

    /// <summary>
    /// Clears an extension.
    /// </summary>
    public TBuilder ClearExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension) {
      ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions.ClearField(extension.Descriptor);
      return ThisBuilder;
    }

    /// <summary>
    /// Called by subclasses to parse an unknown field or an extension.
    /// </summary>
    /// <returns>true unless the tag is an end-group tag</returns>
    [CLSCompliant(false)]
    protected override bool ParseUnknownField(CodedInputStream input,
        ExtensionRegistry extensionRegistry, uint tag) {
      FieldSet extensions = MessageBeingBuilt.Extensions;

      WireFormat.WireType wireType = WireFormat.GetTagWireType(tag);
      int fieldNumber = WireFormat.GetTagFieldNumber(tag);
      IGeneratedExtensionLite extension = extensionRegistry[DefaultInstanceForType, fieldNumber];

      bool unknown = false;
      bool packed = false;
      if (extension == null) {
        unknown = true;  // Unknown field.
      } else if (wireType == FieldMappingAttribute.WireTypeFromFieldType(extension.Descriptor.FieldType, false /* isPacked */)) {
        packed = false;  // Normal, unpacked value.
      } else if (extension.Descriptor.IsRepeated &&
                 //?? just returns true ?? extension.Descriptor.type.isPackable() &&
                 wireType == FieldMappingAttribute.WireTypeFromFieldType(extension.Descriptor.FieldType, true /* isPacked */)) {
        packed = true;  // Packed value.
      } else {
        unknown = true;  // Wrong wire type.
      }

      if (unknown) {  // Unknown field or wrong wire type.  Skip.
        return input.SkipField(tag);
      }

      if (packed) {
        int length = (int)Math.Min(int.MaxValue, input.ReadRawVarint32());
        int limit = input.PushLimit(length);
        if (extension.Descriptor.FieldType == FieldType.Enum) {
          while (!input.ReachedLimit) {
            int rawValue = input.ReadEnum();
            Object value =
                extension.Descriptor.EnumType.FindValueByNumber(rawValue);
            if (value == null) {
              // If the number isn't recognized as a valid value for this
              // enum, drop it (don't even add it to unknownFields).
              return true;
            }
            extensions.AddRepeatedField(extension.Descriptor, value);
          }
        } else {
          while (!input.ReachedLimit) {
            Object value = input.ReadPrimitiveField(extension.Descriptor.FieldType);
            extensions.AddRepeatedField(extension.Descriptor, value);
          }
        }
        input.PopLimit(limit);
      } else {
        Object value;
        switch (extension.Descriptor.MappedType) {
          case MappedType.Message: {
            IBuilderLite subBuilder = null;
            if (!extension.Descriptor.IsRepeated) {
              IMessageLite existingValue = extensions[extension.Descriptor] as IMessageLite;
              if (existingValue != null) {
                subBuilder = existingValue.WeakToBuilder();
              }
            }
            if (subBuilder == null) {
              subBuilder = extension.MessageDefaultInstance.WeakCreateBuilderForType();
            }
            if (extension.Descriptor.FieldType == FieldType.Group) {
              input.ReadGroup(extension.Number, subBuilder, extensionRegistry);
            } else {
              input.ReadMessage(subBuilder, extensionRegistry);
            }
            value = subBuilder.WeakBuild();
            break;
          }
          case MappedType.Enum:
            int rawValue = input.ReadEnum();
            value = extension.Descriptor.EnumType.FindValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // drop it.
            if (value == null) {
              return true;
            }
            break;
          default:
            value = input.ReadPrimitiveField(extension.Descriptor.FieldType);
            break;
        }

        if (extension.Descriptor.IsRepeated) {
          extensions.AddRepeatedField(extension.Descriptor, value);
        } else {
          extensions[extension.Descriptor] = value;
        }
      }

      return true;
    }

    #region Reflection

    public object this[IFieldDescriptorLite field, int index] {
      set {
        if (field.IsExtension) {
          ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
          message.Extensions[field, index] = value;
        } else {
          throw new NotSupportedException("Not supported in the lite runtime.");
        }
      }
    }

    public object this[IFieldDescriptorLite field] {
      set {
        if (field.IsExtension) {
          ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
          message.Extensions[field] = value;
        } else {
          throw new NotSupportedException("Not supported in the lite runtime.");
        }
      }
    }

    public TBuilder ClearField(IFieldDescriptorLite field) {
      if (field.IsExtension) {
        ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
        message.Extensions.ClearField(field);
        return ThisBuilder;
      } else {
        throw new NotSupportedException("Not supported in the lite runtime.");
      }
    }

    public TBuilder AddRepeatedField(IFieldDescriptorLite field, object value) {
      if (field.IsExtension) {
        ExtendableMessageLite<TMessage, TBuilder> message = MessageBeingBuilt;
        message.Extensions.AddRepeatedField(field, value);
        return ThisBuilder;
      } else {
        throw new NotSupportedException("Not supported in the lite runtime.");
      }
    }

    protected void MergeExtensionFields(ExtendableMessageLite<TMessage, TBuilder> other) {
      MessageBeingBuilt.Extensions.MergeFrom(other.Extensions);
    }
    #endregion
  }
}
