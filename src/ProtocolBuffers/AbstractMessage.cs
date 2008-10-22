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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Implementation of the non-generic IMessage interface as far as possible.
  /// </summary>
  public abstract class AbstractMessage<TMessage, TBuilder> : IMessage<TMessage, TBuilder> 
      where TMessage : AbstractMessage<TMessage, TBuilder> 
      where TBuilder : AbstractBuilder<TMessage, TBuilder> {
    /// <summary>
    /// The serialized size if it's already been computed, or null
    /// if we haven't computed it yet.
    /// </summary>
    private int? memoizedSize = null;

    #region Unimplemented members of IMessage
    public abstract MessageDescriptor DescriptorForType { get; }
    public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
    public abstract bool HasField(FieldDescriptor field);
    public abstract object this[FieldDescriptor field] { get; }
    public abstract int GetRepeatedFieldCount(FieldDescriptor field);
    public abstract object this[FieldDescriptor field, int index] { get; }
    public abstract UnknownFieldSet UnknownFields { get; }
    public abstract TMessage DefaultInstanceForType { get; }
    public abstract TBuilder CreateBuilderForType();
    #endregion
    
    public IBuilder WeakCreateBuilderForType() {
      return CreateBuilderForType();
    }

    public IMessage WeakDefaultInstanceForType {
      get { return DefaultInstanceForType; }
    }

    public virtual bool IsInitialized {
      get {
        // Check that all required fields are present.
        foreach (FieldDescriptor field in DescriptorForType.Fields) {
          if (field.IsRequired && !HasField(field)) {
            return false;
          }
        }

        // Check that embedded messages are initialized.
        foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields) {
          FieldDescriptor field = entry.Key;
          if (field.MappedType == MappedType.Message) {
            if (field.IsRepeated) {
              // We know it's an IList<T>, but not the exact type - so
              // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
              foreach (IMessage element in (IEnumerable) entry.Value) {
                if (!element.IsInitialized) {
                  return false;
                }
              }
            } else {
              if (!((IMessage)entry.Value).IsInitialized) {
                return false;
              }
            }
          }
        }
        return true;
      }
    }

    public sealed override string ToString() {
      return TextFormat.PrintToString(this);
    }

    public virtual void WriteTo(CodedOutputStream output) {
      foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // We know it's an IList<T>, but not the exact type - so
          // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
          foreach (object element in (IEnumerable)entry.Value) {
            output.WriteField(field.FieldType, field.FieldNumber, element);
          }
        } else {
          output.WriteField(field.FieldType, field.FieldNumber, entry.Value);
        }
      }

      UnknownFieldSet unknownFields = UnknownFields;
      if (DescriptorForType.Options.MessageSetWireFormat) {
        unknownFields.WriteAsMessageSetTo(output);
      } else {
        unknownFields.WriteTo(output);
      }
    }

    public virtual int SerializedSize {
      get {
        if (memoizedSize != null) {
          return memoizedSize.Value;
        }

        int size = 0;
        foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields) {
          FieldDescriptor field = entry.Key;
          if (field.IsRepeated) {
            foreach (object element in (IEnumerable) entry.Value) {
              size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, element);
            }
          } else {
            size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, entry.Value);
          }
        }

        UnknownFieldSet unknownFields = UnknownFields;
        if (DescriptorForType.Options.MessageSetWireFormat) {
          size += unknownFields.SerializedSizeAsMessageSet;
        } else {
          size += unknownFields.SerializedSize;
        }

        memoizedSize = size;
        return size;
      }
    }

    public ByteString ToByteString() {
      ByteString.CodedBuilder output = new ByteString.CodedBuilder(SerializedSize);
      WriteTo(output.CodedOutput);
      return output.Build();
    }

    public byte[] ToByteArray() {
      byte[] result = new byte[SerializedSize];
      CodedOutputStream output = CodedOutputStream.CreateInstance(result);
      WriteTo(output);
      output.CheckNoSpaceLeft();
      return result;
    }

    public void WriteTo(Stream output) {
      CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
      WriteTo(codedOutput);
      codedOutput.Flush();
    }

    public override bool Equals(object other) {
      if (other == this) {
        return true;
      }
      IMessage otherMessage = other as IMessage;
      if (otherMessage == null || otherMessage.DescriptorForType != DescriptorForType) {
        return false;
      }
      return Dictionaries.Equals(AllFields, otherMessage.AllFields);
    }

    public override int GetHashCode() {
      int hash = 41;
      hash = (19 * hash) + DescriptorForType.GetHashCode();
      hash = (53 * hash) + Dictionaries.GetHashCode(AllFields);
      return hash;
    }
  }
}
