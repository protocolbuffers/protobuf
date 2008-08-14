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

    internal FieldAccessorTable FieldAccesseorsFromBuilder {
      get { return InternalFieldAccessors; }
    }

    protected abstract FieldAccessorTable InternalFieldAccessors { get; }

    public override MessageDescriptor DescriptorForType {
      get { return InternalFieldAccessors.Descriptor; }
    }

    internal IDictionary<FieldDescriptor, Object> GetMutableFieldMap() {

      // Use a SortedList so we'll end up serializing fields in order
      var ret = new SortedList<FieldDescriptor, object>();
      MessageDescriptor descriptor = DescriptorForType;
      foreach (FieldDescriptor field in descriptor.Fields) {
        IFieldAccessor accessor = InternalFieldAccessors[field];
        if (field.IsRepeated) {
          if (accessor.GetRepeatedCount(this) != 0) {
            ret[field] = accessor.GetValue(this);
          }
        } else if (HasField(field)) {
          ret[field] = accessor.GetValue(this);
        }
      }
      return ret;
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return Dictionaries.AsReadOnly(GetMutableFieldMap()); }
    }

    public override bool HasField(FieldDescriptor field) {
      return InternalFieldAccessors[field].Has(this);
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return InternalFieldAccessors[field].GetRepeatedCount(this);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return InternalFieldAccessors[field].GetRepeatedValue(this, index); }
    }

    public override object this[FieldDescriptor field] {
      get { return InternalFieldAccessors[field].GetValue(this); }
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
