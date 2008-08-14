using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers {
  public abstract class ExtendableMessage<TMessage,TBuilder> : GeneratedMessage<TMessage,TBuilder>
    where TMessage : GeneratedMessage<TMessage, TBuilder>
    where TBuilder : IBuilder<TMessage> {

    protected ExtendableMessage() {}
    private readonly FieldSet extensions = FieldSet.CreateFieldSet();

    /// <summary>
    /// Access for the builder.
    /// </summary>
    internal FieldSet Extensions {
      get { return extensions; }
    }

    /// <summary>
    /// Checks if a singular extension is present.
    /// </summary>
    public bool HasExtension<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension) {
      return extensions.HasField(extension.Descriptor);
    }

    /// <summary>
    /// Returns the number of elements in a repeated extension.
    /// </summary>
    public int GetExtensionCount<TExtension>(GeneratedExtensionBase<TMessage, IList<TExtension>> extension) {
      return extensions.GetRepeatedFieldCount(extension.Descriptor);
    }

    /// <summary>
    /// Returns the value of an extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension) {
      object value = extensions[extension.Descriptor];
      if (value == null) {
        return (TExtension) extension.MessageDefaultInstance;
      } else {
        return (TExtension) extension.FromReflectionType(value);
      }
    }

    /// <summary>
    /// Returns one element of a repeated extension.
    /// </summary>
    public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TMessage, IList<TExtension>> extension, int index) {
      return (TExtension) extension.SingularFromReflectionType(extensions[extension.Descriptor, index]);
    }

    /// <summary>
    /// Called by subclasses to check if all extensions are initialized.
    /// </summary>
    protected bool ExtensionsAreInitialized {
      get { return extensions.IsInitialized; }      
    }

    #region Reflection
    public override IDictionary<FieldDescriptor, object> AllFields {
      get {
        IDictionary<FieldDescriptor, object> result = GetMutableFieldMap();
        foreach(KeyValuePair<FieldDescriptor, object> entry in extensions.AllFields) {
          result[entry.Key] = entry.Value;
        }
        return Dictionaries.AsReadOnly(result);
      }
    }

    public override bool HasField(FieldDescriptor field) {
      if (field.IsExtension) {
        VerifyContainingType(field);
        return extensions.HasField(field);
      } else {
        return base.HasField(field);
      }
    }

    public override object this[FieldDescriptor field] {
      get {
        if (field.IsExtension) {
          VerifyContainingType(field);
          object value = extensions[field];
          if (value == null) {
            // Lacking an ExtensionRegistry, we have no way to determine the
            // extension's real type, so we return a DynamicMessage.
            // TODO(jonskeet): Work out what this means
            return DynamicMessage.GetDefaultInstance(field.MessageType);
          } else {
            return value;
          }
        } else {
          return base[field];
        }
      }
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      if (field.IsExtension) {
        VerifyContainingType(field);
        return extensions.GetRepeatedFieldCount(field);
      } else {
        return base.GetRepeatedFieldCount(field);
      }
    }

    public override object this[FieldDescriptor field, int index] {
      get {
        if (field.IsExtension) {
          VerifyContainingType(field);
          return extensions[field, index];
        } else {
          return base[field, index];
        }
      }
    }
  
    internal void VerifyContainingType(FieldDescriptor field) {
      if (field.ContainingType != DescriptorForType) {
        throw new ArgumentException("FieldDescriptor does not match message type.");
      }
    }
    #endregion

    /// <summary>
    /// Used by subclasses to serialize extensions. Extension ranges may be
    /// interleaves with field numbers, but we must write them in canonical
    /// (sorted by field number) order. This class helps us to write individual
    /// ranges of extensions at once.
    /// 
    /// TODO(jonskeet): See if we can improve this in terms of readability.
    /// </summary>
    protected class ExtensionWriter {
      readonly IEnumerator<KeyValuePair<FieldDescriptor, object>> iterator;
      readonly FieldSet extensions;
      KeyValuePair<FieldDescriptor, object>? next = null;

      internal ExtensionWriter(ExtendableMessage<TMessage, TBuilder> message) {
        extensions = message.extensions;
        iterator = message.extensions.GetEnumerator();
        if (iterator.MoveNext()) {
          next = iterator.Current;
        }
      }

      public void WriteUntil(int end, CodedOutputStream output) {
        while (next != null && next.Value.Key.FieldNumber < end) {
          extensions.WriteField(next.Value.Key, next.Value.Value, output);
          if (iterator.MoveNext()) {
            next = iterator.Current;
          } else {
            next = null;
          }
        }
      }
    }

    protected ExtensionWriter CreateExtensionWriter(ExtendableMessage<TMessage, TBuilder> message) {
      return new ExtensionWriter(message);
    }

    /// <summary>
    /// Called by subclasses to compute the size of extensions.
    /// </summary>
    protected int ExtensionsSerializedSize {
      get { return extensions.SerializedSize; }
    }

    internal void VerifyExtensionContainingType<TExtension>(GeneratedExtensionBase<TMessage, TExtension> extension) {
      if (extension.Descriptor.ContainingType != DescriptorForType) {
        // This can only happen if someone uses unchecked operations.
        throw new ArgumentException("Extension is for type \"" + extension.Descriptor.ContainingType.FullName
          + "\" which does not match message type \"" + DescriptorForType.FullName + "\".");
      }
    }
  }
}
