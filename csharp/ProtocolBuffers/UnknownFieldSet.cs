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
using System.IO;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers {
  public class UnknownFieldSet {

    private static readonly UnknownFieldSet defaultInstance = new UnknownFieldSet(new Dictionary<int, UnknownField>());

    private readonly IDictionary<int, UnknownField> fields;

    private UnknownFieldSet(IDictionary<int, UnknownField> fields) {
      this.fields = fields;
    }

    /// <summary>
    /// Creates a new unknown field set builder.
    /// </summary>
    public static Builder CreateBuilder() {
      return new Builder();
    }

    /// <summary>
    /// Creates a new unknown field set builder 
    /// and initialize it from <paramref name="original"/>.
    /// </summary>
    public static Builder CreateBuilder(UnknownFieldSet original) {
      return new Builder().MergeFrom(original);
    }

    public static UnknownFieldSet DefaultInstance {
      get { return defaultInstance; }
    }

    /// <summary>
    /// Returns a read-only view of the mapping from field numbers to values.
    /// </summary>
    public IDictionary<int, UnknownField> FieldDictionary {
      get { return Dictionaries.AsReadOnly(fields); }
    }

    /// <summary>
    /// Checks whether or not the given field number is present in the set.
    /// </summary>
    public bool HasField(int field) {
      return fields.ContainsKey(field);
    }

    /// <summary>
    /// Fetches a field by number, returning an empty field if not present.
    /// Never returns null.
    /// </summary>
    public UnknownField this[int number] {
      get {
        UnknownField ret;
        if (!fields.TryGetValue(number, out ret)) {
          ret = UnknownField.DefaultInstance;
        }
        return ret;
      }
    }

    /// <summary>
    /// Serializes the set and writes it to <paramref name="output"/>.
    /// </summary>
    public void WriteTo(CodedOutputStream output) {
      foreach (KeyValuePair<int, UnknownField> entry in fields) {
        entry.Value.WriteTo(entry.Key, output);
      }
    }

    /// <summary>
    /// Gets the number of bytes required to encode this set.
    /// </summary>
    public int SerializedSize { 
      get {
        int result = 0;
        foreach (KeyValuePair<int, UnknownField> entry in fields) {
          result += entry.Value.GetSerializedSize(entry.Key);
        }
        return result;
      } 
    }

    /// <summary>
    /// Converts the set to a string in protocol buffer text format. This
    /// is just a trivial wrapper around TextFormat.PrintToString.
    /// </summary>
    public override String ToString() {
      return TextFormat.PrintToString(this);
    }

    /// <summary>
    /// Serializes the message to a ByteString and returns it. This is
    /// just a trivial wrapper around WriteTo(CodedOutputStream).
    /// </summary>
    /// <returns></returns>
    public ByteString ToByteString() {
      ByteString.CodedBuilder codedBuilder = new ByteString.CodedBuilder(SerializedSize);
      WriteTo(codedBuilder.CodedOutput);
      return codedBuilder.Build();
    }

    /// <summary>
    /// Serializes the message to a byte array and returns it. This is
    /// just a trivial wrapper around WriteTo(CodedOutputStream).
    /// </summary>
    /// <returns></returns>
    public byte[] ToByteArray() {
      byte[] data = new byte[SerializedSize];
      CodedOutputStream output = CodedOutputStream.CreateInstance(data);
      WriteTo(output);
      output.CheckNoSpaceLeft();
      return data;
    }

    /// <summary>
    /// Serializes the message and writes it to <paramref name="output"/>. This is
    /// just a trivial wrapper around WriteTo(CodedOutputStream).
    /// </summary>
    /// <param name="output"></param>
    public void WriteTo(Stream output) {
      CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
      WriteTo(codedOutput);
      codedOutput.Flush();
    }

    /// <summary>
    /// Serializes the set and writes it to <paramref name="output"/> using
    /// the MessageSet wire format.
    /// </summary>
    public void WriteAsMessageSetTo(CodedOutputStream output) {
      foreach (KeyValuePair<int, UnknownField> entry in fields) {
        entry.Value.WriteAsMessageSetExtensionTo(entry.Key, output);
      }
    }

    /// <summary>
    /// Gets the number of bytes required to encode this set using the MessageSet
    /// wire format.
    /// </summary>
    public int SerializedSizeAsMessageSet {
      get {
        int result = 0;
        foreach (KeyValuePair<int, UnknownField> entry in fields) {
          result += entry.Value.GetSerializedSizeAsMessageSetExtension(entry.Key);
        }
        return result;
      }
    }


    /// <summary>
    /// Parses an UnknownFieldSet from the given input.
    /// </summary>
    public static UnknownFieldSet ParseFrom(CodedInputStream input) {
      return CreateBuilder().MergeFrom(input).Build();
    }

    /// <summary>
    /// Parses an UnknownFieldSet from the given data.
    /// </summary>
    public static UnknownFieldSet ParseFrom(ByteString data) {
      return CreateBuilder().MergeFrom(data).Build();
    }

    /// <summary>
    /// Parses an UnknownFieldSet from the given data.
    /// </summary>
    public static UnknownFieldSet ParseFrom(byte[] data) {
      return CreateBuilder().MergeFrom(data).Build();
    }

    /// <summary>
    /// Parses an UnknownFieldSet from the given input.
    /// </summary>
    public static UnknownFieldSet ParseFrom(Stream input) {
      return CreateBuilder().MergeFrom(input).Build();
    }

    public class Builder
    {
      /// <summary>
      /// Mapping from number to field. Note that by using a SortedList we ensure
      /// that the fields will be serialized in ascending order.
      /// </summary>
      private IDictionary<int, UnknownField> fields = new SortedList<int, UnknownField>();

      // Optimization:  We keep around a builder for the last field that was
      // modified so that we can efficiently add to it multiple times in a
      // row (important when parsing an unknown repeated field).
      int lastFieldNumber;
      UnknownField.Builder lastField;

      internal Builder() {
      }

      /// <summary>
      /// Returns a field builder for the specified field number, including any values
      /// which already exist.
      /// </summary>
      private UnknownField.Builder GetFieldBuilder(int number) {
        if (lastField != null) {
          if (number == lastFieldNumber) {
            return lastField;
          }
          // Note: AddField() will reset lastField and lastFieldNumber.
          AddField(lastFieldNumber, lastField.Build());
        }
        if (number == 0) {
          return null;
        }

        lastField = UnknownField.CreateBuilder();
        UnknownField existing;
        if (fields.TryGetValue(number, out existing)) {
          lastField.MergeFrom(existing);
        }
        lastFieldNumber = number;
        return lastField;
      }

      /// <summary>
      /// Build the UnknownFieldSet and return it. Once this method has been called,
      /// this instance will no longer be usable. Calling any method after this
      /// will throw a NullReferenceException.
      /// </summary>
      public UnknownFieldSet Build() {
        GetFieldBuilder(0);  // Force lastField to be built.
        UnknownFieldSet result = fields.Count == 0 ? DefaultInstance : new UnknownFieldSet(fields);
        fields = null;
        return result;
      }

      /// <summary>
      /// Adds a field to the set. If a field with the same number already exists, it
      /// is replaced.
      /// </summary>
      public Builder AddField(int number, UnknownField field) {
        if (number == 0) {
          throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
        }
        if (lastField != null && lastFieldNumber == number) {
          // Discard this.
          lastField = null;
          lastFieldNumber = 0;
        }
        fields[number] = field;
        return this;
      }

      /// <summary>
      /// Resets the builder to an empty set.
      /// </summary>
      public Builder Clear() {
        fields.Clear();
        lastFieldNumber = 0;
        lastField = null;
        return this;
      }
      
      /// <summary>
      /// Parse an entire message from <paramref name="input"/> and merge
      /// its fields into this set.
      /// </summary>
      public Builder MergeFrom(CodedInputStream input) {
        while (true) {
          uint tag = input.ReadTag();
          if (tag == 0 || !MergeFieldFrom(tag, input)) {
            break;
          }
        }
        return this;
      }

        /// <summary>
        /// Parse a single field from <paramref name="input"/> and merge it
        /// into this set.
        /// </summary>
        /// <param name="tag">The field's tag number, which was already parsed.</param>
        /// <param name="input">The coded input stream containing the field</param>
        /// <returns>false if the tag is an "end group" tag, true otherwise</returns>
      public bool MergeFieldFrom(uint tag, CodedInputStream input) {
        int number = WireFormat.GetTagFieldNumber(tag);
        switch (WireFormat.GetTagWireType(tag)) {
          case WireFormat.WireType.Varint:
            // TODO(jonskeet): Check this is correct (different to Java)
            GetFieldBuilder(number).AddVarint(input.ReadUInt64());
            return true;
          case WireFormat.WireType.Fixed64:
            GetFieldBuilder(number).AddFixed64(input.ReadFixed64());
            return true;
          case WireFormat.WireType.LengthDelimited:
            GetFieldBuilder(number).AddLengthDelimited(input.ReadBytes());
            return true;
          case WireFormat.WireType.StartGroup: {
            Builder subBuilder = CreateBuilder();
            input.ReadUnknownGroup(number, subBuilder);
            GetFieldBuilder(number).AddGroup(subBuilder.Build());
            return true;
          }
          case WireFormat.WireType.EndGroup:
            return false;
          case WireFormat.WireType.Fixed32:
            GetFieldBuilder(number).AddFixed32(input.ReadFixed32());
            return true;
          default:
            throw InvalidProtocolBufferException.InvalidWireType();
        }
      }

      /// <summary>
      /// Parses <paramref name="input"/> as an UnknownFieldSet and merge it
      /// with the set being built. This is just a small wrapper around
      /// MergeFrom(CodedInputStream).
      /// </summary>
      public Builder MergeFrom(Stream input) {
        CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
        MergeFrom(codedInput);
        codedInput.CheckLastTagWas(0);
        return this;
      }

      /// <summary>
      /// Parses <paramref name="data"/> as an UnknownFieldSet and merge it
      /// with the set being built. This is just a small wrapper around
      /// MergeFrom(CodedInputStream).
      /// </summary>
      public Builder MergeFrom(ByteString data) {
        CodedInputStream input = data.CreateCodedInput();
        MergeFrom(input);
        input.CheckLastTagWas(0);
        return this;
      }

      /// <summary>
      /// Parses <paramref name="data"/> as an UnknownFieldSet and merge it
      /// with the set being built. This is just a small wrapper around
      /// MergeFrom(CodedInputStream).
      /// </summary>
      public Builder MergeFrom(byte[] data) {
        CodedInputStream input = CodedInputStream.CreateInstance(data);
        MergeFrom(input);
        input.CheckLastTagWas(0);
        return this;
      }

      /// <summary>
      /// Convenience method for merging a new field containing a single varint
      /// value.  This is used in particular when an unknown enum value is
      /// encountered.
      /// </summary>
      public Builder MergeVarintField(int number, ulong value) {
        if (number == 0) {
          throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
        }
        GetFieldBuilder(number).AddVarint(value);
        return this;
      }

      /// <summary>
      /// Merges the fields from <paramref name="other"/> into this set.
      /// If a field number exists in both sets, the values in <paramref name="other"/>
      /// will be appended to the values in this set.
      /// </summary>
      public Builder MergeFrom(UnknownFieldSet other) {
        if (other != DefaultInstance) {
          foreach(KeyValuePair<int, UnknownField> entry in other.fields) {
            MergeField(entry.Key, entry.Value);
          }
        }
        return this;
      }

      /// <summary>
      /// Checks if the given field number is present in the set.
      /// </summary>
      public bool HasField(int number) {
        if (number == 0) {
          throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
        }
        return number == lastFieldNumber || fields.ContainsKey(number);
      }

      /// <summary>
      /// Adds a field to the unknown field set. If a field with the same
      /// number already exists, the two are merged.
      /// </summary>
      public Builder MergeField(int number, UnknownField field) {
        if (number == 0) {
          throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
        }
        if (HasField(number)) {
          GetFieldBuilder(number).MergeFrom(field);
        } else {
          // Optimization:  We could call getFieldBuilder(number).mergeFrom(field)
          // in this case, but that would create a copy of the Field object.
          // We'd rather reuse the one passed to us, so call AddField() instead.
          AddField(number, field);
        }
        return this;
      }

    }
  }
}
