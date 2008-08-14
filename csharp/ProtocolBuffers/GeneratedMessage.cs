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
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;
using System.Collections;

namespace Google.ProtocolBuffers {
  
  /// <summary>
  /// All generated protocol message classes extend this class. It implements
  /// most of the IMessage interface using reflection. Users
  /// can ignore this class as an implementation detail.
  /// </summary>
  public abstract class GeneratedMessage<TMessage, TBuilder> : AbstractMessage<TMessage, TBuilder>
      where TMessage : GeneratedMessage<TMessage, TBuilder> 
      where TBuilder : GeneratedBuilder<TMessage, TBuilder> {

    private UnknownFieldSet unknownFields = UnknownFieldSet.DefaultInstance;

    /// <summary>
    /// Returns the message as a TMessage.
    /// </summary>
    protected abstract TMessage ThisMessage { get; }

    internal FieldAccessorTable<TMessage, TBuilder> FieldAccessorsFromBuilder {
      get { return InternalFieldAccessors; }
    }

    protected abstract FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors { get; }

    public override MessageDescriptor DescriptorForType {
      get { return InternalFieldAccessors.Descriptor; }
    }

    internal IDictionary<FieldDescriptor, Object> GetMutableFieldMap() {

      // Use a SortedList so we'll end up serializing fields in order
      var ret = new SortedList<FieldDescriptor, object>();
      MessageDescriptor descriptor = DescriptorForType;
      foreach (FieldDescriptor field in descriptor.Fields) {
        IFieldAccessor<TMessage, TBuilder> accessor = InternalFieldAccessors[field];
        if (field.IsRepeated) {
          if (accessor.GetRepeatedCount(ThisMessage) != 0) {
            ret[field] = accessor.GetValue(ThisMessage);
          }
        } else if (HasField(field)) {
          ret[field] = accessor.GetValue(ThisMessage);
        }
      }
      return ret;
    }

    public override bool IsInitialized {
      get {
        // Check that all required fields are present.
        foreach (FieldDescriptor field in DescriptorForType.Fields) {
          if (field.IsRequired && !HasField(field)) {
            return false;
          }
        }

        // Check that embedded messages are initialized.
        // This code is similar to that in AbstractMessage, but we don't
        // fetch all the field values - just the ones we need to.
        foreach (FieldDescriptor field in DescriptorForType.Fields) {
          if (field.MappedType == MappedType.Message) {
            if (field.IsRepeated) {
              // We know it's an IList<T>, but not the exact type - so
              // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
              foreach (IMessage element in (IEnumerable) this[field]) {
                if (!element.IsInitialized) {
                  return false;
                }
              }
            } else {
              if (!((IMessage) this[field]).IsInitialized) {
                return false;
              }
            }
          }
        }
        return true;
      }
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return Dictionaries.AsReadOnly(GetMutableFieldMap()); }
    }

    public override bool HasField(FieldDescriptor field) {
      return InternalFieldAccessors[field].Has(ThisMessage);
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return InternalFieldAccessors[field].GetRepeatedCount(ThisMessage);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return InternalFieldAccessors[field].GetRepeatedValue(ThisMessage, index); }
    }

    public override object this[FieldDescriptor field] {
      get { return InternalFieldAccessors[field].GetValue(ThisMessage); }
    }

    public override UnknownFieldSet UnknownFields {
      get { return unknownFields; }
    }

    /// <summary>
    /// Replaces the set of unknown fields for this message. This should
    /// only be used before a message is built, by the builder. (In the
    /// Java code it is private, but the builder is nested so has access
    /// to it.)
    /// </summary>
    internal void SetUnknownFields(UnknownFieldSet fieldSet) {
      unknownFields = fieldSet;
    }    
  }
}
