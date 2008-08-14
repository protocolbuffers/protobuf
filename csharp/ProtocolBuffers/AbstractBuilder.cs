using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using System.Collections;
using System.IO;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Implementation of the non-generic IMessage interface as far as possible.
  /// TODO(jonskeet): Make this generic, to avoid so much casting in DynamicMessage.
  /// </summary>
  public abstract class AbstractBuilder : IBuilder {
    #region Unimplemented members of IBuilder
    public abstract bool Initialized { get; }
    public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
    public abstract object this[FieldDescriptor field] { get; set; }
    public abstract MessageDescriptor DescriptorForType { get; }
    public abstract int GetRepeatedFieldCount(FieldDescriptor field);
    public abstract object this[FieldDescriptor field, int index] { get; set; }
    public abstract bool HasField(FieldDescriptor field);
    #endregion

    #region New abstract methods to be overridden by implementations, allow explicit interface implementation
    protected abstract IMessage BuildImpl();
    protected abstract IMessage BuildPartialImpl();
    protected abstract IBuilder CloneImpl();
    protected abstract IMessage DefaultInstanceForTypeImpl { get; }
    protected abstract IBuilder ClearFieldImpl(FieldDescriptor field);
    protected abstract IBuilder AddRepeatedFieldImpl(FieldDescriptor field, object value);
    #endregion

    #region Methods simply proxying to the "Impl" methods, explicitly implementing IBuilder
    IMessage IBuilder.Build() {
      return BuildImpl();
    }

    IMessage IBuilder.BuildPartial() {
      return BuildPartialImpl();
    }

    IBuilder IBuilder.Clone() {
      return CloneImpl();
    }
    
    IMessage IBuilder.DefaultInstanceForType {
      get { return DefaultInstanceForTypeImpl; }
    }

    public abstract IBuilder CreateBuilderForField(FieldDescriptor field);

    IBuilder IBuilder.ClearField(FieldDescriptor field) {
      return ClearFieldImpl(field);
    }

    IBuilder IBuilder.AddRepeatedField(FieldDescriptor field, object value) {
      return AddRepeatedFieldImpl(field, value);
    }
    #endregion

    public virtual IBuilder Clear() {
      foreach(FieldDescriptor field in AllFields.Keys) {
        ClearFieldImpl(field);
      }
      return this;
    }

    public virtual IBuilder MergeFrom(IMessage other) {
      if (other.DescriptorForType != DescriptorForType) {
        throw new ArgumentException("MergeFrom(Message) can only merge messages of the same type.");
      }

      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the Message interface itself cannot enforce immutability of
      //   implementations).
      // TODO(jonskeet):  Provide a function somewhere called makeDeepCopy()
      //   which allows people to make secure deep copies of messages.
      foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // Concatenate repeated fields
          foreach (object element in (IEnumerable) entry.Value) {
            AddRepeatedFieldImpl(field, element);
          }
        } else if (field.MappedType == MappedType.Message) {
          // Merge singular messages
          IMessage existingValue = (IMessage) this[field];
          if (existingValue == existingValue.DefaultInstanceForType) {
            this[field] = entry.Value;
          } else {
            this[field] = existingValue.CreateBuilderForType()
                                       .MergeFrom(existingValue)
                                       .MergeFrom((IMessage) entry.Value)
                                       .Build();
          }
        } else {
          // Overwrite simple values
          this[field] = entry.Value;
        }
      }
      return this;
    }

    IBuilder IBuilder.MergeFrom(CodedInputStream input) {
      return MergeFromImpl(input, ExtensionRegistry.Empty);
    }

    protected virtual IBuilder MergeFromImpl(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder(UnknownFields);
      FieldSet.MergeFrom(input, unknownFields, extensionRegistry, this);
      UnknownFields = unknownFields.Build();
      return this;
    }

    IBuilder IBuilder.MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      return MergeFromImpl(input, extensionRegistry);
    }

    IBuilder IBuilder.MergeUnknownFields(UnknownFieldSet unknownFields) {
      UnknownFields = UnknownFieldSet.CreateBuilder(UnknownFields)
          .MergeFrom(unknownFields)
          .Build();
      return this;
    }

    IBuilder IBuilder.MergeFrom(ByteString data) {
      CodedInputStream input = data.CreateCodedInput();
      ((IBuilder)this).MergeFrom(input);
      input.CheckLastTagWas(0);
      return this;
    }

    IBuilder IBuilder.MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = data.CreateCodedInput();
      ((IBuilder)this).MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return this;
    }

    IBuilder IBuilder.MergeFrom(byte[] data) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      ((IBuilder)this).MergeFrom(input);
      input.CheckLastTagWas(0);
      return this;
    }

    IBuilder IBuilder.MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      ((IBuilder)this).MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return this;
    }

    IBuilder IBuilder.MergeFrom(Stream input) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      ((IBuilder)this).MergeFrom(codedInput);
      codedInput.CheckLastTagWas(0);
      return this;
    }

    IBuilder IBuilder.MergeFrom(Stream input, ExtensionRegistry extensionRegistry) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      ((IBuilder)this).MergeFrom(codedInput, extensionRegistry);
      codedInput.CheckLastTagWas(0);
      return this;
    }

    public abstract UnknownFieldSet UnknownFields { get; set; }
    
    public IBuilder SetField(FieldDescriptor field, object value) {
      this[field] = value;
      return this;
    }

    public IBuilder SetRepeatedField(FieldDescriptor field, int index, object value) {
      this[field, index] = value;
      return this;
    }
  }
}
