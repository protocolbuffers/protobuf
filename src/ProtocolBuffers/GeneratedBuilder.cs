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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// All generated protocol message builder classes extend this class. It implements
  /// most of the IBuilder interface using reflection. Users can ignore this class
  /// as an implementation detail.
  /// </summary>
  public abstract class GeneratedBuilder<TMessage, TBuilder> : AbstractBuilder<TMessage, TBuilder>
      where TMessage : GeneratedMessage <TMessage, TBuilder>
      where TBuilder : GeneratedBuilder<TMessage, TBuilder> {

    /// <summary>
    /// Returns the message being built at the moment.
    /// </summary>
    protected abstract TMessage MessageBeingBuilt { get; }

    protected internal FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors {
      get { return MessageBeingBuilt.FieldAccessorsFromBuilder; }
    }

    public override bool IsInitialized {
      get { return MessageBeingBuilt.IsInitialized; }
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return MessageBeingBuilt.AllFields; }
    }

    public override object this[FieldDescriptor field] {
      get {
        // For repeated fields, the underlying list object is still modifiable at this point.
        // Make sure not to expose the modifiable list to the caller.
        return field.IsRepeated
          ? InternalFieldAccessors[field].GetRepeatedWrapper(ThisBuilder)
          : MessageBeingBuilt[field];
      }
      set {
        InternalFieldAccessors[field].SetValue(ThisBuilder, value);
      }
    }

    /// <summary>
    /// Adds all of the specified values to the given collection.
    /// </summary>
    protected void AddRange<T>(IEnumerable<T> source, IList<T> destination) {
      List<T> list = destination as List<T>;
      if (list != null) {
        list.AddRange(source);
      } else {
        foreach (T element in source) {
          destination.Add(element);
        }
      }
    }

    /// <summary>
    /// Called by derived classes to parse an unknown field.
    /// </summary>
    /// <returns>true unless the tag is an end-group tag</returns>
    protected virtual bool ParseUnknownField(CodedInputStream input, UnknownFieldSet.Builder unknownFields,
                                     ExtensionRegistry extensionRegistry, uint tag) {
      return unknownFields.MergeFieldFrom(tag, input);
    }

    public override MessageDescriptor DescriptorForType {
      get { return MessageBeingBuilt.DescriptorForType; }
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return MessageBeingBuilt.GetRepeatedFieldCount(field);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return MessageBeingBuilt[field, index]; }
      set { InternalFieldAccessors[field].SetRepeated(ThisBuilder, index, value); }
    }

    public override bool HasField(FieldDescriptor field) {
      return MessageBeingBuilt.HasField(field);
    }

    public override IBuilder CreateBuilderForField(FieldDescriptor field) {
      return InternalFieldAccessors[field].CreateBuilder();
    }

    public override TBuilder ClearField(FieldDescriptor field) {
      InternalFieldAccessors[field].Clear(ThisBuilder);
      return ThisBuilder;
    }

    public override TBuilder MergeFrom(TMessage other) {
      if (other.DescriptorForType != InternalFieldAccessors.Descriptor) {
        throw new ArgumentException("Message type mismatch");
      }

      foreach (KeyValuePair<FieldDescriptor, object> entry in other.AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // Concatenate repeated fields
          foreach (object element in (IEnumerable)entry.Value) {
            AddRepeatedField(field, element);
          }
        } else if (field.MappedType == MappedType.Message && HasField(field)) {
          // Merge singular embedded messages
          IMessage oldValue = (IMessage)this[field];
          this[field] = oldValue.WeakCreateBuilderForType()
              .WeakMergeFrom(oldValue)
              .WeakMergeFrom((IMessage)entry.Value)
              .WeakBuildPartial();
        } else {
          // Just overwrite
          this[field] = entry.Value;
        }
      }
      return ThisBuilder;
    }

    public override TBuilder MergeUnknownFields(UnknownFieldSet unknownFields) {
      TMessage result = MessageBeingBuilt;
      result.SetUnknownFields(UnknownFieldSet.CreateBuilder(result.UnknownFields)
          .MergeFrom(unknownFields)
          .Build());
      return ThisBuilder;
    }

    public override TBuilder AddRepeatedField(FieldDescriptor field, object value) {
      InternalFieldAccessors[field].AddRepeated(ThisBuilder, value);
      return ThisBuilder;
    }

    /// <summary>
    /// Like Build(), but will wrap UninitializedMessageException in
    /// InvalidProtocolBufferException.
    /// </summary>
    public TMessage BuildParsed() {
      if (!IsInitialized) {
        throw new UninitializedMessageException(MessageBeingBuilt).AsInvalidProtocolBufferException();
      }
      return BuildPartial();
    }

    /// <summary>
    /// Implementation of <see cref="IBuilder{TMessage, TBuilder}.Build" />.
    /// </summary>
    public override TMessage Build() {
      if (!IsInitialized) {
        throw new UninitializedMessageException(MessageBeingBuilt);
      }
      return BuildPartial();
    }

    public override UnknownFieldSet UnknownFields {
      get { return MessageBeingBuilt.UnknownFields; }
      set { MessageBeingBuilt.SetUnknownFields(value); }
    }
  }
}
