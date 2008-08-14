using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// A class which represents an arbitrary set of fields of some message type.
  /// This is used to implement DynamicMessage, and also to represent extensions
  /// in GeneratedMessage. This class is internal, since outside users should probably
  /// be using DynamicMessage.
  /// 
  /// As in the Java implementation, this class goes against the rest of the framework
  /// in terms of mutability. Instead of having a mutable Builder class and an immutable
  /// FieldSet class, FieldSet just has a MakeImmutable() method. This is safe so long as
  /// all callers are careful not to let a mutable FieldSet escape into the open. This would
  /// be impossible to guarantee if this were a public class, of course.
  /// 
  /// All repeated fields are stored as IList[object] even 
  /// </summary>
  internal class FieldSet {

    private static readonly FieldSet defaultInstance = new FieldSet(new Dictionary<FieldDescriptor, object>()).MakeImmutable();

    private IDictionary<FieldDescriptor, object> fields;

    private FieldSet(IDictionary<FieldDescriptor, object> fields) {
      this.fields = fields;
    }

    public static FieldSet CreateFieldSet() {
      return new FieldSet(new Dictionary<FieldDescriptor, object>());
    }

    /// <summary>
    /// Makes this FieldSet immutable, and returns it for convenience. Any
    /// mutable repeated fields are made immutable, as well as the map itself.
    /// </summary>
    internal FieldSet MakeImmutable() {
      // First check if we have any repeated values
      bool hasRepeats = false;
      foreach (object value in fields.Values) {
        IList<object> list = value as IList<object>;
        if (list != null && !list.IsReadOnly) {
          hasRepeats = true;
          break;
        }
      }

      if (hasRepeats) {
        var tmp = new SortedList<FieldDescriptor, object>();
        foreach (KeyValuePair<FieldDescriptor, object> entry in fields) {
          IList<object> list = entry.Value as IList<object>;
          tmp[entry.Key] = list == null ? entry.Value : Lists.AsReadOnly(list);
        }
        fields = tmp;
      }

      fields = Dictionaries.AsReadOnly(fields);

      return this;
    }

    /// <summary>
    /// Returns the default, immutable instance with no fields defined.
    /// </summary>
    internal static FieldSet DefaultInstance {
      get { return defaultInstance; }
    }

    /// <summary>
    /// Returns an immutable mapping of fields. Note that although the mapping itself
    /// is immutable, the entries may not be (i.e. any repeated values are represented by
    /// mutable lists). The behaviour is not specified if the contents are mutated.
    /// </summary>
    internal IDictionary<FieldDescriptor, object> AllFields {
      get { return Dictionaries.AsReadOnly(fields); }
    }

    /// <summary>
    /// See <see cref="IMessage.HasField"/>.
    /// </summary>
    public bool HasField(FieldDescriptor field) {
      if (field.IsRepeated) {
        throw new ArgumentException("HasField() can only be called on non-repeated fields.");
      }

      return fields.ContainsKey(field);
    }

    // TODO(jonskeet): Should this be in UnknownFieldSet.Builder really? Or CodedInputStream?
    internal static void MergeFrom(CodedInputStream input, UnknownFieldSet.Builder unknownFields,
         ExtensionRegistry extensionRegistry, IBuilder builder) {
      while (true) {
        uint tag = input.ReadTag();
        if (tag == 0) {
          break;
        }
        if (!MergeFieldFrom(input, unknownFields, extensionRegistry,
                            builder, tag)) {
          // end group tag
          break;
        }
      }
    }

    // TODO(jonskeet): Should this be in UnknownFieldSet.Builder really? Or CodedInputStream?
    /// <summary>
    /// Like <see cref="MergeFrom(CodedInputStream, UnknownFieldSet.Builder, ExtensionRegistry, IBuilder)" />
    /// but parses a single field.
    /// </summary>
    /// <param name="input">The input to read the field from</param>
    /// <param name="unknownFields">The set of unknown fields to add the newly-read field to, if it's not a known field</param>
    /// <param name="extensionRegistry">Registry to use when an extension field is encountered</param>
    /// <param name="builder">Builder to merge field into, if it's a known field</param>
    /// <param name="tag">The tag, which should already have been read from the input</param>
    /// <returns>true unless the tag is an end-group tag</returns>
    internal static bool MergeFieldFrom(CodedInputStream input, UnknownFieldSet.Builder unknownFields,
        ExtensionRegistry extensionRegistry, IBuilder builder, uint tag) {
      MessageDescriptor type = builder.DescriptorForType;
      if (type.Options.MessageSetWireFormat && tag == WireFormat.MessageSetTag.ItemStart) {
        MergeMessageSetExtensionFromCodedStream(input, unknownFields, extensionRegistry, builder);
        return true;
      }

      WireFormat.WireType wireType = WireFormat.GetTagWireType(tag);
      int fieldNumber = WireFormat.GetTagFieldNumber(tag);

      FieldDescriptor field;
      IMessage defaultFieldInstance = null;

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
      if (field == null || wireType != WireFormat.FieldTypeToWireFormatMap[field.FieldType]) {
        return unknownFields.MergeFieldFrom(tag, input);
      }

      object value;
      switch (field.FieldType) {
        case FieldType.Group:
        case FieldType.Message: {
            IBuilder subBuilder;
            if (defaultFieldInstance != null) {
              subBuilder = defaultFieldInstance.CreateBuilderForType();
            } else {
              subBuilder = builder.CreateBuilderForField(field);
            }
            if (!field.IsRepeated) {
              subBuilder.MergeFrom((IMessage) builder[field]);
            }
            if (field.FieldType == FieldType.Group) {
              input.ReadGroup(field.FieldNumber, subBuilder, extensionRegistry);
            } else {
              input.ReadMessage(subBuilder, extensionRegistry);
            }
            value = subBuilder.Build();
            break;
          }
        case FieldType.Enum: {
            int rawValue = input.ReadEnum();
            value = field.EnumType.FindValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // drop it.
            if (value == null) {
              unknownFields.MergeVarintField(fieldNumber, (ulong) rawValue);
              return true;
            }
            break;
          }
        default:
          value = input.ReadPrimitiveField(field.FieldType);
          break;
      }
      if (field.IsRepeated) {
        builder.AddRepeatedField(field, value);
      } else {
        builder[field] = value;
      } 
      return true;
    }

    // TODO(jonskeet): Should this be in UnknownFieldSet.Builder really? Or CodedInputStream?
    /// <summary>
    /// Called by MergeFieldFrom to parse a MessageSet extension.
    /// </summary>
    private static void MergeMessageSetExtensionFromCodedStream(CodedInputStream input, 
        UnknownFieldSet.Builder unknownFields, 
        ExtensionRegistry extensionRegistry, 
        IBuilder builder) {
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
      IBuilder subBuilder = null;
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
              subBuilder = extension.DefaultInstance.CreateBuilderForType();
              IMessage originalMessage = (IMessage) builder[field];
              if (originalMessage != null) {
                subBuilder.MergeFrom(originalMessage);
              }
              if (rawBytes != null) {
                // We already encountered the message.  Parse it now.
                // TODO(jonskeet): Check this is okay. It's subtly different from the Java, as it doesn't create an input stream from rawBytes.
                // In fact, why don't we just call MergeFrom(rawBytes)? And what about the extension registry?
                subBuilder.MergeFrom(rawBytes.CreateCodedInput());
                rawBytes = null;
              }
            } else {
              // Unknown extension number.  If we already saw data, put it
              // in rawBytes.
              if (rawBytes != null) {
                unknownFields.MergeField(typeId, 
                    UnknownField.CreateBuilder()
                        .AddLengthDelimited(rawBytes)
                        .Build());
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
            unknownFields.MergeField(typeId,
              UnknownField.CreateBuilder()
                .AddLengthDelimited(input.ReadBytes())
                .Build());
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
        builder[field] = subBuilder.Build();
      }
    }


    /// <summary>
    /// Clears all fields.
    /// </summary>
    internal void Clear() {
      fields.Clear();
    }

    /// <summary>
    /// See <see cref="IMessage.Item(FieldDescriptor)"/>
    /// </summary>
    /// <remarks>
    /// If the field is not set, the behaviour when fetching this property varies by field type:
    /// <list>
    /// <item>For singular message values, null is returned.</item>
    /// <item>For singular non-message values, the default value of the field is returned.</item>
    /// <item>For repeated values, an empty immutable list is returned. This will be compatible
    /// with IList[object], regardless of the type of the repeated item.</item>
    /// </list>
    /// This method returns null if the field is a singular message type
    /// and is not set; in this case it is up to the caller to fetch the 
    /// message's default instance. For repeated fields of message types, 
    /// an empty collection is returned. For repeated fields of non-message
    /// types, null is returned.
    /// <para />
    /// When setting this property, any list values are copied, and each element is checked
    /// to ensure it is of an appropriate type.
    /// </remarks>
    /// 
    internal object this[FieldDescriptor field] {
      get {
        object result;
        if (fields.TryGetValue(field, out result)) {
          return result;
        }

        // This will just do the right thing
        return field.DefaultValue;
      }
      set {
        if (field.IsRepeated) {
          List<object> list = value as List<object>;
          if (list == null) {
            throw new ArgumentException("Wrong object type used with protocol message reflection.");
          }

          // Wrap the contents in a new list so that the caller cannot change
          // the list's contents after setting it.
          List<object> newList = new List<object>(list);
          foreach (object element in newList) {
            VerifyType(field, element);
          }
          value = newList;
        }
        else {
          VerifyType(field, value);
        }
        fields[field] = value;
      }
    }

    /// <summary>
    /// See <see cref="IMessage.Item(FieldDescriptor,int)" />
    /// </summary>
    internal object this[FieldDescriptor field, int index] {
      get {
        if (!field.IsRepeated) {
          throw new ArgumentException("Indexer specifying field and index can only be called on repeated fields.");
        }

        return ((IList<object>) this[field])[index];
      }
      set {
        if (!field.IsRepeated) {
          throw new ArgumentException("Indexer specifying field and index can only be called on repeated fields.");
        }
        VerifyType(field, value);
        object list;
        if (!fields.TryGetValue(field, out list)) {
          throw new ArgumentOutOfRangeException();
        }
        ((IList<object>) list)[index] = value;
      }
    }

    /// <summary>
    /// See <see cref="IBuilder.AddRepeatedField" />
    /// </summary>
    internal void AddRepeatedField(FieldDescriptor field, object value) {
      if (!field.IsRepeated) {
        throw new ArgumentException("AddRepeatedField can only be called on repeated fields.");
      }
      VerifyType(field, value);
      object list;
      if (!fields.TryGetValue(field, out list)) {
        list = new List<object>();
        fields[field] = list;
      }
      ((IList<object>) list).Add(value);
    }

    /// <summary>
    /// Returns an enumerator for the field map. Used to write the fields out.
    /// </summary>
    internal IEnumerator<KeyValuePair<FieldDescriptor, object>> GetEnumerator() {
      return fields.GetEnumerator();
    }

    /// <summary>
    /// See <see cref="IMessage.IsInitialized" />
    /// </summary>
    /// <remarks>
    /// Since FieldSet itself does not have any way of knowing about
    /// required fields that aren't actually present in the set, it is up
    /// to the caller to check for genuinely required fields. This property
    /// merely checks that any messages present are themselves initialized.
    /// </remarks>
    internal bool IsInitialized {
      get {
        foreach (KeyValuePair<FieldDescriptor, object> entry in fields) {
          FieldDescriptor field = entry.Key;
          if (field.MappedType == MappedType.Message) {
            if (field.IsRepeated) {
              foreach(IMessage message in (IEnumerable) entry.Value) {
                if (!message.IsInitialized) {
                  return false;
                }
              }
            } else {
              if (!((IMessage) entry.Value).IsInitialized) {
                return false;
              }
            }
          }
        }
        return true;
      }
    }

    /// <summary>
    /// Verifies whether all the required fields in the specified message
    /// descriptor are present in this field set, as well as whether
    /// all the embedded messages are themselves initialized.
    /// </summary>
    internal bool IsInitializedWithRespectTo(MessageDescriptor type) {
      foreach (FieldDescriptor field in type.Fields) {
        if (field.IsRequired && !HasField(field)) {
          return false;
        }
      }
      return IsInitialized;
    }

    /// <summary>
    /// See <see cref="IBuilder.ClearField" />
    /// </summary>
    public void ClearField(FieldDescriptor field) {
      fields.Remove(field);
    }

    /// <summary>
    /// See <see cref="IMessage.GetRepeatedFieldCount" />
    /// </summary>
    public int GetRepeatedFieldCount(FieldDescriptor field) {
      if (!field.IsRepeated) {
        throw new ArgumentException("GetRepeatedFieldCount() can only be called on repeated fields.");
      }

      return ((IList<object>) this[field]).Count;
    }

    /// <summary>
    /// Implementation of both <c>MergeFrom</c> methods.
    /// </summary>
    /// <param name="otherFields"></param>
    private void MergeFields(IEnumerable<KeyValuePair<FieldDescriptor, object>> otherFields) {
      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the IMessage interface itself cannot enforce immutability of
      //   implementations).
      // TODO(jonskeet):  Provide a function somewhere called MakeDeepCopy()
      //   which allows people to make secure deep copies of messages.

      foreach (KeyValuePair<FieldDescriptor, object> entry in otherFields) {
        FieldDescriptor field = entry.Key;
        object existingValue;
        fields.TryGetValue(field, out existingValue);
        if (field.IsRepeated) {
          if (existingValue == null) {
            existingValue = new List<object>();
            fields[field] = existingValue;
          }
          IList<object> list = (IList<object>) existingValue;
          foreach (object otherValue in (IEnumerable) entry.Value) {
            list.Add(otherValue);
          }
        } else if (field.MappedType == MappedType.Message && existingValue != null) {
          IMessage existingMessage = (IMessage)existingValue;
          IMessage merged = existingMessage.CreateBuilderForType()
              .MergeFrom(existingMessage)
              .MergeFrom((IMessage)entry.Value)
              .Build();
          this[field] = merged;
        } else {
          this[field] = entry.Value;
        }
      }
    }

    /// <summary>
    /// See <see cref="IBuilder.MergeFrom(IMessage)" />
    /// </summary>
    public void MergeFrom(IMessage other) {
      MergeFields(other.AllFields);
    }

    /// <summary>
    /// Like <see cref="MergeFrom(IMessage)"/>, but merges from another <c>FieldSet</c>.
    /// </summary>
    public void MergeFrom(FieldSet other) {
      MergeFields(other.fields);
    }

    /// <summary>
    /// See <see cref="IMessage.WriteTo(CodedOutputStream)" />.
    /// </summary>
    public void WriteTo(CodedOutputStream output) {
      foreach (KeyValuePair<FieldDescriptor, object> entry in fields) {
        WriteField(entry.Key, entry.Value, output);
      }
    }

    /// <summary>
    /// Writes a single field to a CodedOutputStream.
    /// </summary>
    public void WriteField(FieldDescriptor field, Object value, CodedOutputStream output) {
      if (field.IsExtension && field.ContainingType.Options.MessageSetWireFormat) {
        output.WriteMessageSetExtension(field.FieldNumber, (IMessage) value);
      } else {
        if (field.IsRepeated) {
          foreach (object element in (IEnumerable) value) {
            output.WriteField(field.FieldType, field.FieldNumber, element);
          }
        } else {
          output.WriteField(field.FieldType, field.FieldNumber, value);
        }
      }
    }

    /// <summary>
    /// See <see cref="IMessage.SerializedSize" />. It's up to the caller to
    /// cache the resulting size if desired.
    /// </summary>
    public int SerializedSize {
      get {
        int size = 0;
        foreach (KeyValuePair<FieldDescriptor, object> entry in fields) {
          FieldDescriptor field = entry.Key;
          object value = entry.Value;

          if (field.IsExtension && field.ContainingType.Options.MessageSetWireFormat) {
            size += CodedOutputStream.ComputeMessageSetExtensionSize(field.FieldNumber, (IMessage) value);
          } else {
            if (field.IsRepeated) {
              foreach (object element in (IEnumerable) value) {
                size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, element);
              }
            } else {
              size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, value);
            }
          }
        }
        return size;
      }
    }

    /// <summary>
    /// Verifies that the given object is of the correct type to be a valid
    /// value for the given field.
    /// </summary>
    /// <remarks>
    /// For repeated fields, this checks if the object is of the right
    /// element type, not whether it's a list.
    /// </remarks>
    /// <exception cref="ArgumentException">The value is not of the right type.</exception>
    private static void VerifyType(FieldDescriptor field, object value) {
      bool isValid = false;
      switch (field.MappedType) {
        case MappedType.Int32:       isValid = value is int;    break;
        case MappedType.Int64:       isValid = value is long;   break;
        case MappedType.UInt32:      isValid = value is uint;   break;
        case MappedType.UInt64:      isValid = value is ulong;  break;
        case MappedType.Single:      isValid = value is float;  break;
        case MappedType.Double:      isValid = value is double; break;
        case MappedType.Boolean:     isValid = value is bool;   break;
        case MappedType.String:      isValid = value is string; break;
        case MappedType.ByteString:  isValid = value is ByteString; break;        
        case MappedType.Enum:
          EnumValueDescriptor enumValue = value as EnumValueDescriptor;
          isValid = enumValue != null && enumValue.EnumDescriptor == field.EnumType;
          break;
        case MappedType.Message:
          IMessage messageValue = value as IMessage;
          isValid = messageValue != null && messageValue.DescriptorForType == field.MessageType;
          break;
      }

      if (!isValid) {
        // When chaining calls to SetField(), it can be hard to tell from
        // the stack trace which exact call failed, since the whole chain is
        // considered one line of code.  So, let's make sure to include the
        // field name and other useful info in the exception.
        throw new ArgumentException("Wrong object type used with protocol message reflection. "
            + "Message type \"" + field.ContainingType.FullName
            + "\", field \"" + (field.IsExtension ? field.FullName : field.Name)
            + "\", value was type \"" + value.GetType().Name + "\".");
      }
    }     
  }
}
