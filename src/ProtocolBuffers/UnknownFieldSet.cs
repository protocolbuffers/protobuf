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

using System;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Used to keep track of fields which were seen when parsing a protocol message
  /// but whose field numbers or types are unrecognized. This most frequently
  /// occurs when new fields are added to a message type and then messages containing
  /// those fields are read by old software that was built before the new types were
  /// added.
  /// 
  /// Every message contains an UnknownFieldSet.
  /// 
  /// Most users will never need to use this class directly.
  /// </summary>
  public sealed class UnknownFieldSet : IMessageLite {

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
    /// Converts the set to a string in protocol buffer text format. This
    /// is just a trivial wrapper around TextFormat.PrintToString.
    /// </summary>
    public void PrintTo(TextWriter writer) {
      TextFormat.Print(this, writer);
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

    public override bool Equals(object other) {
      if (ReferenceEquals(this, other)) {
        return true;
      }
      UnknownFieldSet otherSet = other as UnknownFieldSet;
      return otherSet != null && Dictionaries.Equals(fields, otherSet.fields);
    }

    public override int GetHashCode() {
      return Dictionaries.GetHashCode(fields);
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

    #region IMessageLite Members

    public bool IsInitialized {
      get { return fields != null; }
    }

    public void WriteDelimitedTo(Stream output) {
      CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
      codedOutput.WriteRawVarint32((uint) SerializedSize);
      WriteTo(codedOutput);
      codedOutput.Flush();
    }

    public IBuilderLite WeakCreateBuilderForType() {
      return new Builder();
    }

    public IBuilderLite WeakToBuilder() {
      return new Builder(fields);
    }

    public IMessageLite WeakDefaultInstanceForType {
      get { return defaultInstance; }
    }

    #endregion

    /// <summary>
    /// Builder for UnknownFieldSets.
    /// </summary>
    public sealed class Builder : IBuilderLite
    {
      /// <summary>
      /// Mapping from number to field. Note that by using a SortedList we ensure
      /// that the fields will be serialized in ascending order.
      /// </summary>
      private IDictionary<int, UnknownField> fields;
      // Optimization:  We keep around a builder for the last field that was
      // modified so that we can efficiently add to it multiple times in a
      // row (important when parsing an unknown repeated field).
      private int lastFieldNumber;
      private UnknownField.Builder lastField;

      internal Builder() {
        fields = new SortedList<int, UnknownField>();
      }

      internal Builder(IDictionary<int, UnknownField> dictionary) {
        fields = new SortedList<int, UnknownField>(dictionary); 
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
      [CLSCompliant(false)]
      public bool MergeFieldFrom(uint tag, CodedInputStream input) {
        int number = WireFormat.GetTagFieldNumber(tag);
        switch (WireFormat.GetTagWireType(tag)) {
          case WireFormat.WireType.Varint:
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
#pragma warning disable 0612
            input.ReadUnknownGroup(number, subBuilder);
#pragma warning restore 0612
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
      [CLSCompliant(false)]
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

      internal void MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry, IBuilder builder) {
        while (true) {
          uint tag = input.ReadTag();
          if (tag == 0) {
            break;
          }

          if (!MergeFieldFrom(input, extensionRegistry, builder, tag)) {
            // end group tag
            break;
          }
        }
      }

      /// <summary>
      /// Like <see cref="MergeFrom(CodedInputStream, ExtensionRegistry, IBuilder)" />
      /// but parses a single field.
      /// </summary>
      /// <param name="input">The input to read the field from</param>
      /// <param name="extensionRegistry">Registry to use when an extension field is encountered</param>
      /// <param name="builder">Builder to merge field into, if it's a known field</param>
      /// <param name="tag">The tag, which should already have been read from the input</param>
      /// <returns>true unless the tag is an end-group tag</returns>
      internal bool MergeFieldFrom(CodedInputStream input,
          ExtensionRegistry extensionRegistry, IBuilder builder, uint tag) {

        MessageDescriptor type = builder.DescriptorForType;
        if (type.Options.MessageSetWireFormat && tag == WireFormat.MessageSetTag.ItemStart) {
          MergeMessageSetExtensionFromCodedStream(input, extensionRegistry, builder);
          return true;
        }

        WireFormat.WireType wireType = WireFormat.GetTagWireType(tag);
        int fieldNumber = WireFormat.GetTagFieldNumber(tag);

        FieldDescriptor field;
        IMessageLite defaultFieldInstance = null;

        if (type.IsExtensionNumber(fieldNumber)) {
          ExtensionInfo extension = extensionRegistry[type, fieldNumber];
          if (extension == null) {
            field = null;
          } else {
            field = extension.Descriptor;
            defaultFieldInstance = extension.DefaultInstance;
          }
        } else {
          field = type.FindFieldByNumber(fieldNumber);
        }

        // Unknown field or wrong wire type. Skip.
        if (field == null || wireType != WireFormat.GetWireType(field)) {
          return MergeFieldFrom(tag, input);
        }

        if (field.IsPacked) {
          int length = (int)input.ReadRawVarint32();
          int limit = input.PushLimit(length);
          if (field.FieldType == FieldType.Enum) {
            while (!input.ReachedLimit) {
              int rawValue = input.ReadEnum();
              object value = field.EnumType.FindValueByNumber(rawValue);
              if (value == null) {
                // If the number isn't recognized as a valid value for this
                // enum, drop it (don't even add it to unknownFields).
                return true;
              }
              builder.WeakAddRepeatedField(field, value);
            }
          } else {
            while (!input.ReachedLimit) {
              Object value = input.ReadPrimitiveField(field.FieldType);
              builder.WeakAddRepeatedField(field, value);
            }
          }
          input.PopLimit(limit);
        } else {
          object value;
          switch (field.FieldType) {
            case FieldType.Group:
            case FieldType.Message: {
                IBuilderLite subBuilder;
                if (defaultFieldInstance != null) {
                  subBuilder = defaultFieldInstance.WeakCreateBuilderForType();
                } else {
                  subBuilder = builder.CreateBuilderForField(field);
                }
                if (!field.IsRepeated) {
                  subBuilder.WeakMergeFrom((IMessageLite)builder[field]);
                }
                if (field.FieldType == FieldType.Group) {
                  input.ReadGroup(field.FieldNumber, subBuilder, extensionRegistry);
                } else {
                  input.ReadMessage(subBuilder, extensionRegistry);
                }
                value = subBuilder.WeakBuild();
                break;
              }
            case FieldType.Enum: {
                int rawValue = input.ReadEnum();
                value = field.EnumType.FindValueByNumber(rawValue);
                // If the number isn't recognized as a valid value for this enum,
                // drop it.
                if (value == null) {
                  MergeVarintField(fieldNumber, (ulong)rawValue);
                  return true;
                }
                break;
              }
            default:
              value = input.ReadPrimitiveField(field.FieldType);
              break;
          }
          if (field.IsRepeated) {
            builder.WeakAddRepeatedField(field, value);
          } else {
            builder[field] = value;
          }
        }
        return true;
      }

      /// <summary>
      /// Called by MergeFieldFrom to parse a MessageSet extension.
      /// </summary>
      private void MergeMessageSetExtensionFromCodedStream(CodedInputStream input,
          ExtensionRegistry extensionRegistry, IBuilder builder) {
        MessageDescriptor type = builder.DescriptorForType;

        // The wire format for MessageSet is:
        //   message MessageSet {
        //     repeated group Item = 1 {
        //       required int32 typeId = 2;
        //       required bytes message = 3;
        //     }
        //   }
        // "typeId" is the extension's field number.  The extension can only be
        // a message type, where "message" contains the encoded bytes of that
        // message.
        //
        // In practice, we will probably never see a MessageSet item in which
        // the message appears before the type ID, or where either field does not
        // appear exactly once.  However, in theory such cases are valid, so we
        // should be prepared to accept them.

        int typeId = 0;
        ByteString rawBytes = null;  // If we encounter "message" before "typeId"
        IBuilderLite subBuilder = null;
        FieldDescriptor field = null;

        while (true) {
          uint tag = input.ReadTag();
          if (tag == 0) {
            break;
          }

          if (tag == WireFormat.MessageSetTag.TypeID) {
            typeId = input.ReadInt32();
            // Zero is not a valid type ID.
            if (typeId != 0) {
              ExtensionInfo extension = extensionRegistry[type, typeId];
              if (extension != null) {
                field = extension.Descriptor;
                subBuilder = extension.DefaultInstance.WeakCreateBuilderForType();
                IMessageLite originalMessage = (IMessageLite)builder[field];
                if (originalMessage != null) {
                  subBuilder.WeakMergeFrom(originalMessage);
                }
                if (rawBytes != null) {
                  // We already encountered the message.  Parse it now.
                  // TODO(jonskeet): Check this is okay. It's subtly different from the Java, as it doesn't create an input stream from rawBytes.
                  // In fact, why don't we just call MergeFrom(rawBytes)? And what about the extension registry?
                  subBuilder.WeakMergeFrom(rawBytes.CreateCodedInput());
                  rawBytes = null;
                }
              } else {
                // Unknown extension number.  If we already saw data, put it
                // in rawBytes.
                if (rawBytes != null) {
                  MergeField(typeId, UnknownField.CreateBuilder().AddLengthDelimited(rawBytes).Build());
                  rawBytes = null;
                }
              }
            }
          } else if (tag == WireFormat.MessageSetTag.Message) {
            if (typeId == 0) {
              // We haven't seen a type ID yet, so we have to store the raw bytes for now.
              rawBytes = input.ReadBytes();
            } else if (subBuilder == null) {
              // We don't know how to parse this.  Ignore it.
              MergeField(typeId, UnknownField.CreateBuilder().AddLengthDelimited(input.ReadBytes()).Build());
            } else {
              // We already know the type, so we can parse directly from the input
              // with no copying.  Hooray!
              input.ReadMessage(subBuilder, extensionRegistry);
            }
          } else {
            // Unknown tag.  Skip it.
            if (!input.SkipField(tag)) {
              break;  // end of group
            }
          }
        }

        input.CheckLastTagWas(WireFormat.MessageSetTag.ItemEnd);

        if (subBuilder != null) {
          builder[field] = subBuilder.WeakBuild();
        }
      }

      #region IBuilderLite Members

      bool IBuilderLite.IsInitialized {
        get { return fields != null; }
      }

      IBuilderLite IBuilderLite.WeakClear() {
        return Clear();
      }

      IBuilderLite IBuilderLite.WeakMergeFrom(IMessageLite message) {
        return MergeFrom((UnknownFieldSet)message);
      }

      IBuilderLite IBuilderLite.WeakMergeFrom(ByteString data) {
        return MergeFrom(data);
      }

      IBuilderLite IBuilderLite.WeakMergeFrom(ByteString data, ExtensionRegistry registry) {
        return MergeFrom(data);
      }

      IBuilderLite IBuilderLite.WeakMergeFrom(CodedInputStream input) {
        return MergeFrom(input);
      }

      IBuilderLite IBuilderLite.WeakMergeFrom(CodedInputStream input, ExtensionRegistry registry) {
        return MergeFrom(input);
      }

      IMessageLite IBuilderLite.WeakBuild() {
        return Build();
      }

      IMessageLite IBuilderLite.WeakBuildPartial() {
        return Build();
      }

      IBuilderLite IBuilderLite.WeakClone() {
        return Build().WeakToBuilder();
      }

      IMessageLite IBuilderLite.WeakDefaultInstanceForType {
        get { return DefaultInstance; }
      }

      #endregion
    }
  }
}
