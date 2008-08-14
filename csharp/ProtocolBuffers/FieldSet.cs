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
  /// </summary>
  internal class FieldSet {

    private static readonly FieldSet defaultInstance = new FieldSet(new Dictionary<FieldDescriptor, object>()).MakeImmutable();

    private IDictionary<FieldDescriptor, object> fields;

    private FieldSet(IDictionary<FieldDescriptor, object> fields) {
      this.fields = fields;
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

    // TODO(jonskeet): Should this be in UnknownFieldSet.Builder really?
    internal static void MergeFrom(CodedInputStream input,
         UnknownFieldSet.Builder unknownFields,
         ExtensionRegistry extensionRegistry,
         IBuilder builder) {

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

    // TODO(jonskeet): Should this be in UnknownFieldSet.Builder really?
    internal static bool MergeFieldFrom(CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistry extensionRegistry,
        IBuilder builder,
        uint tag) {
      throw new NotImplementedException();
    }

    /// <summary>
    /// Clears all fields.
    /// </summary>
    internal void Clear() {
      fields.Clear();
    }

    /// <summary>
    /// <see cref="IMessage.Item(FieldDescriptor)"/>
    /// </summary>
    /// <remarks>
    /// If the field is not set, the behaviour when fetching this property varies by field type:
    /// <list>
    /// <item>For singular message values, null is returned.</item>
    /// <item>For singular non-message values, the default value of the field is returned.</item>
    /// <item>For repeated values, an empty immutable list is returned.</item>
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
    /// <see cref="IMessage.Item(FieldDescriptor,int)" />
    /// </summary>
    internal object this[FieldDescriptor field, int index] {
      get {
        if (!field.IsRepeated) {
          throw new ArgumentException("Indexer specifying field and index can only be called on repeated fields.");
        }

        return ((List<object>)this[field])[index];
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
        ((List<object>) list)[index] = value;
      }
    }

    /// <summary>
    /// <see cref="IBuilder.AddRepeatedField" />
    /// </summary>
    /// <param name="field"></param>
    /// <param name="value"></param>
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
      ((List<object>) list).Add(value);
    }

    /// <summary>
    /// <see cref="IMessage.IsInitialized" />
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
    /// <see cref="IBuilder.ClearField" />
    /// </summary>
    public void ClearField(FieldDescriptor field) {
      fields.Remove(field);
    }

    /// <summary>
    /// <see cref="IMessage.GetRepeatedFieldCount" />
    /// </summary>
    public int GetRepeatedFieldCount(FieldDescriptor field) {
      if (!field.IsRepeated) {
        throw new ArgumentException("GetRepeatedFieldCount() can only be called on repeated fields.");
      }

      return ((List<object>) this[field]).Count;
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
