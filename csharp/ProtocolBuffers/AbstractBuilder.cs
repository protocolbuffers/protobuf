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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Implementation of the non-generic IMessage interface as far as possible.
  /// </summary>
  public abstract class AbstractBuilder<TMessage, TBuilder> : IBuilder<TMessage, TBuilder> 
      where TMessage : AbstractMessage<TMessage, TBuilder>
      where TBuilder : AbstractBuilder<TMessage, TBuilder> {

    protected abstract TBuilder ThisBuilder { get; }
    
    #region Unimplemented members of IBuilder
    public abstract UnknownFieldSet UnknownFields { get; set; }
    public abstract TBuilder MergeFrom(TMessage other);
    public abstract bool IsInitialized { get; }
    public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
    public abstract object this[FieldDescriptor field] { get; set; }
    public abstract MessageDescriptor DescriptorForType { get; }
    public abstract int GetRepeatedFieldCount(FieldDescriptor field);
    public abstract object this[FieldDescriptor field, int index] { get; set; }
    public abstract bool HasField(FieldDescriptor field);
    public abstract TMessage Build();
    public abstract TMessage BuildPartial();
    public abstract TBuilder Clone();
    public abstract TMessage DefaultInstanceForType { get; }
    public abstract IBuilder CreateBuilderForField(FieldDescriptor field);
    public abstract TBuilder ClearField(FieldDescriptor field);
    public abstract TBuilder AddRepeatedField(FieldDescriptor field, object value);
    #endregion

    #region Implementation of methods which don't require type parameter information
    public IMessage WeakBuild() {
      return Build();
    }

    public IBuilder WeakAddRepeatedField(FieldDescriptor field, object value) {
      return AddRepeatedField(field, value);
    }

    public IBuilder WeakClear() {
      return Clear();
    }

    public IBuilder WeakMergeFrom(IMessage message) {
      return MergeFrom(message);
    }

    public IBuilder WeakMergeFrom(CodedInputStream input) {
      return MergeFrom(input);
    }

    public IBuilder WeakMergeFrom(CodedInputStream input, ExtensionRegistry registry) {
      return MergeFrom(input, registry);
    }

    public IBuilder WeakMergeFrom(ByteString data) {
      return MergeFrom(data);
    }

    public IBuilder WeakMergeFrom(ByteString data, ExtensionRegistry registry) {
      return MergeFrom(data, registry);
    }

    public IMessage WeakBuildPartial() {
      return BuildPartial();
    }

    public IBuilder WeakClone() {
      return Clone();
    }

    public IMessage WeakDefaultInstanceForType {
      get { return DefaultInstanceForType; } 
    }

    public IBuilder WeakClearField(FieldDescriptor field) {
      return ClearField(field);
    }
    #endregion

    public TBuilder SetUnknownFields(UnknownFieldSet fields) {
      UnknownFields = fields;
      return ThisBuilder;
    }

    public virtual TBuilder Clear() {
      foreach(FieldDescriptor field in AllFields.Keys) {
        ClearField(field);
      }
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(IMessage other) {
      if (other.DescriptorForType != DescriptorForType) {
        throw new ArgumentException("MergeFrom(IMessage) can only merge messages of the same type.");
      }

      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the Message interface itself cannot enforce immutability of
      //   implementations).
      // TODO(jonskeet):  Provide a function somewhere called makeDeepCopy()
      //   which allows people to make secure deep copies of messages.
      foreach (KeyValuePair<FieldDescriptor, object> entry in other.AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // Concatenate repeated fields
          foreach (object element in (IEnumerable) entry.Value) {
            AddRepeatedField(field, element);
          }
        } else if (field.MappedType == MappedType.Message) {
          // Merge singular messages
          IMessage existingValue = (IMessage) this[field];
          if (existingValue == existingValue.WeakDefaultInstanceForType) {
            this[field] = entry.Value;
          } else {
            this[field] = existingValue.WeakCreateBuilderForType()
                                       .WeakMergeFrom(existingValue)
                                       .WeakMergeFrom((IMessage) entry.Value)
                                       .WeakBuild();
          }
        } else {
          // Overwrite simple values
          this[field] = entry.Value;
        }
      }
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(CodedInputStream input) {
      return MergeFrom(input, ExtensionRegistry.Empty);
    }

    public virtual TBuilder MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder(UnknownFields);
      FieldSet.MergeFrom(input, unknownFields, extensionRegistry, this);
      UnknownFields = unknownFields.Build();
      return ThisBuilder;
    }

    public virtual TBuilder MergeUnknownFields(UnknownFieldSet unknownFields) {
      UnknownFields = UnknownFieldSet.CreateBuilder(UnknownFields)
          .MergeFrom(unknownFields)
          .Build();
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(ByteString data) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(byte[] data) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(Stream input) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput);
      codedInput.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput, extensionRegistry);
      codedInput.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual IBuilder SetField(FieldDescriptor field, object value) {
      this[field] = value;
      return ThisBuilder;
    }

    public virtual IBuilder SetRepeatedField(FieldDescriptor field, int index, object value) {
      this[field, index] = value;
      return ThisBuilder;
    }
  }
}
