using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  public abstract class ExtendableBuilder<TMessage, TBuilder> : GeneratedBuilder<TMessage, TBuilder>
    where TMessage : ExtendableMessage<TMessage, TBuilder>
    where TBuilder : GeneratedBuilder<TMessage, TBuilder> {

    protected ExtendableBuilder() {}

    /// <summary>
    /// Checks if a singular extension is present
    /// </summary>
    public bool HasExtension<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension) {
      return MessageBeingBuilt.HasExtension(extension);
    }

    /// <summary>
    /// Returns the number of elements in a repeated extension.
    /// </summary>
    public int GetExtensionCount<TExtension>(GeneratedExtensionBase<TMessage, IList<TExtension>> extension) {
      return MessageBeingBuilt.GetExtensionCount(extension);
    }

    /// <summary>
    /// Returns the value of an extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension) {
      return MessageBeingBuilt.GetExtension(extension);
    }

    /// <summary>
    /// Returns one element of a repeated extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TMessage, IList<TExtension>> extension, int index) {
      return MessageBeingBuilt.GetExtension(extension, index);
    }

    /// <summary>
    /// Sets the value of an extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> SetExtension<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor] = extension.ToReflectionType(value);
      return this;
    }

    /// <summary>
    /// Sets the value of one element of a repeated extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> SetExtension<TExtension>(
        GeneratedExtensionBase<TMessage, IList<TExtension>> extension, int index, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions[extension.Descriptor, index] = extension.SingularToReflectionType(value);
      return this;
    }

    /// <summary>
    /// Appends a value to a repeated extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> AddExtension<TExtension>(
        GeneratedExtensionBase<TMessage, IList<TExtension>> extension, TExtension value) {
      ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
      message.VerifyExtensionContainingType(extension);
      message.Extensions.AddRepeatedField(extension.Descriptor, extension.SingularToReflectionType(value));
      return this;
    }

    /// <summary>
    /// Clears an extension.
    /// </summary>
    public ExtendableBuilder<TMessage, TBuilder> ClearExtension<TExtension>(
        GeneratedExtensionBase<TMessage, TExtension> extension) {
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
      return FieldSet.MergeFieldFrom(input, unknownFields, extensionRegistry, this, tag);
    }

    // ---------------------------------------------------------------
    // Reflection


    public override object this[FieldDescriptor field, int index] {
      set {
        if (field.IsExtension) {
          ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
          // TODO(jonskeet): Figure out how to call this!
          // message.VerifyExtensionContainingType(field);
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
          // TODO(jonskeet): Figure out how to call this!
          // message.VerifyExtensionContainingType(field);
          message.Extensions[field] = value;
        } else {
          base[field] = value;
        }
      }
    }

    public override IBuilder<TMessage> ClearField(FieldDescriptor field) {
      if (field.IsExtension) {
        ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
        message.VerifyContainingType(field);
        message.Extensions.ClearField(field);
        return this;
      } else {
        return base.ClearField(field);
      }
    }

    public override IBuilder<TMessage> AddRepeatedField(FieldDescriptor field, object value) {
      if (field.IsExtension) {
        ExtendableMessage<TMessage, TBuilder> message = MessageBeingBuilt;
        message.VerifyContainingType(field); 
        message.Extensions.AddRepeatedField(field, value);
        return this;
      } else {
        return base.AddRepeatedField(field, value);
      }
    }
  }
}
