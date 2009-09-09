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
    public abstract TBuilder ToBuilder();
    #endregion
    
    public IBuilder WeakCreateBuilderForType() {
      return CreateBuilderForType();
    }

    public IBuilder WeakToBuilder() {
      return ToBuilder();
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
          IEnumerable valueList = (IEnumerable) entry.Value;
          if (field.IsPacked) {
            output.WriteTag(field.FieldNumber, WireFormat.WireType.LengthDelimited);
            int dataSize = 0;
            foreach (object element in valueList) {
              dataSize += CodedOutputStream.ComputeFieldSizeNoTag(field.FieldType, element);
            }
            output.WriteRawVarint32((uint)dataSize);
            foreach (object element in valueList) {
              output.WriteFieldNoTag(field.FieldType, element);
            }
          } else {
            foreach (object element in valueList) {
              output.WriteField(field.FieldType, field.FieldNumber, element);
            }
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
            IEnumerable valueList = (IEnumerable) entry.Value;
            if (field.IsPacked) {
              int dataSize = 0;
              foreach (object element in valueList) {
                dataSize += CodedOutputStream.ComputeFieldSizeNoTag(field.FieldType, element);
              }
              size += dataSize;
              size += CodedOutputStream.ComputeTagSize(field.FieldNumber);
              size += CodedOutputStream.ComputeRawVarint32Size((uint)dataSize);
            } else {
              foreach (object element in valueList) {
                size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, element);
              }
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

    public void WriteDelimitedTo(Stream output) {
      CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
      codedOutput.WriteRawVarint32((uint) SerializedSize);
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
      return Dictionaries.Equals(AllFields, otherMessage.AllFields) && UnknownFields.Equals(otherMessage.UnknownFields);
    }

    public override int GetHashCode() {
      int hash = 41;
      hash = (19 * hash) + DescriptorForType.GetHashCode();
      hash = (53 * hash) + Dictionaries.GetHashCode(AllFields);
      hash = (29 * hash) + UnknownFields.GetHashCode();
      return hash;
    }
  }
}
