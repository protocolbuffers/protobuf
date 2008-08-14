using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using System.Collections;
using System.IO;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Implementation of the non-generic IMessage interface as far as possible.
  /// </summary>
  public abstract class AbstractBuilder : IBuilder {

    public bool Initialized {
      get { throw new NotImplementedException(); }
    }

    public IDictionary<FieldDescriptor, object> AllFields {
      get { throw new NotImplementedException(); }
    }

    public object this[FieldDescriptor field] {
      get {
        throw new NotImplementedException();
      }
      set {
        throw new NotImplementedException();
      }
    }

    public MessageDescriptor DescriptorForType {
      get { throw new NotImplementedException(); }
    }

    public int GetRepeatedFieldCount(FieldDescriptor field) {
      throw new NotImplementedException();
    }

    public object this[FieldDescriptor field, int index] {
      get {
        throw new NotImplementedException();
      }
      set {
        throw new NotImplementedException();
      }
    }

    public bool HasField(FieldDescriptor field) {
      throw new NotImplementedException();
    }

    public IBuilder Clear() {
      foreach(FieldDescriptor field in AllFields.Keys) {
        ClearField(field);
      }
      return this;
    }

    public IBuilder MergeFrom(IMessage other) {
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
            AddRepeatedField(field, element);
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

    public IMessage Build() {
      throw new NotImplementedException();
    }

    public IMessage BuildPartial() {
      throw new NotImplementedException();
    }

    public IBuilder Clone() {
      throw new NotImplementedException();
    }

    public IBuilder MergeFrom(CodedInputStream input) {
      return MergeFrom(input, ExtensionRegistry.Empty);
    }

    public IBuilder MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder(UnknownFields);
      FieldSet.MergeFrom(input, unknownFields, extensionRegistry, this);
      UnknownFields = unknownFields.Build();
      return this;
    }

    public IMessage DefaultInstanceForType {
      get { throw new NotImplementedException(); }
    }

    public IBuilder NewBuilderForField<TField>(FieldDescriptor field) {
      throw new NotImplementedException();
    }

    public IBuilder ClearField(FieldDescriptor field) {
      throw new NotImplementedException();
    }

    public IBuilder AddRepeatedField(FieldDescriptor field, object value) {
      throw new NotImplementedException();
    }

    public IBuilder MergeUnknownFields(UnknownFieldSet unknownFields) {
      UnknownFields = UnknownFieldSet.CreateBuilder(UnknownFields)
          .MergeFrom(unknownFields)
          .Build();
      return this;
    }

    public UnknownFieldSet UnknownFields {
      get {
        throw new NotImplementedException();
      }
      set {
        throw new NotImplementedException();
      }
    }

    public IBuilder MergeFrom(ByteString data) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return this;
    }

    public IBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return this;
    }

    public IBuilder MergeFrom(byte[] data) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return this;
    }

    public IBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return this;
    }

    public IBuilder MergeFrom(Stream input) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput);
      codedInput.CheckLastTagWas(0);
      return this;
    }

    public IBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput, extensionRegistry);
      codedInput.CheckLastTagWas(0);
      return this;
    }
  }
}
