using System;
using Google.ProtocolBuffers.Descriptors;
using System.Globalization;

namespace Google.ProtocolBuffers.ProtoGen {
  internal abstract class FieldGeneratorBase : SourceGeneratorBase<FieldDescriptor> {
    protected FieldGeneratorBase(FieldDescriptor descriptor)
        : base(descriptor) {
    }

    private static bool AllPrintableAscii(string text) {
      foreach (char c in text) {
        if (c < 0x20 || c > 0x7e) {
          return false;
        }
      }
      return true;
    }

    protected string DefaultValue {
      get {
        string suffix = "";
        switch (Descriptor.FieldType) {
          case FieldType.Float:  suffix = "F"; break;
          case FieldType.Double: suffix = "D"; break;
          case FieldType.Int64:  suffix = "L"; break;
          case FieldType.UInt64: suffix = "UL"; break;
        }
        switch (Descriptor.FieldType) {
          case FieldType.Float:
          case FieldType.Double:
          case FieldType.Int32:
          case FieldType.Int64:
          case FieldType.SInt32:
          case FieldType.SInt64:
          case FieldType.SFixed32:
          case FieldType.SFixed64:
          case FieldType.UInt32:
          case FieldType.UInt64:
          case FieldType.Fixed32:
          case FieldType.Fixed64:
            // The simple Object.ToString converts using the current culture.
            // We want to always use the invariant culture so it's predictable.
            IConvertible value = (IConvertible) Descriptor.DefaultValue;
            return value.ToString(CultureInfo.InvariantCulture) + suffix;
          case FieldType.Bool:
            return (bool) Descriptor.DefaultValue ? "true" : "false";

          case FieldType.Bytes:
            if (!Descriptor.HasDefaultValue) {
              return "pb::ByteString.Empty";
            }
            return string.Format("(pb::ByteString) {0}.Descriptor.Fields[{1}].DefaultValue", GetClassName(Descriptor.ContainingType), Descriptor.Index);
          case FieldType.String:
            if (AllPrintableAscii(Descriptor.Proto.DefaultValue)) {
              // All chars are ASCII and printable.  In this case we only
              // need to escape quotes and backslashes.
              return "\"" + Descriptor.Proto.DefaultValue
                  .Replace("\\", "\\\\")
                  .Replace("'", "\\'")
                  .Replace("\"", "\\\"")
                  + "\"";
            }
            return string.Format("(string) {0}.Descriptor.Fields[{1}].DefaultValue", GetClassName(Descriptor.ContainingType), Descriptor.Index);
          case FieldType.Enum:
            return TypeName + "." + ((EnumValueDescriptor) Descriptor.DefaultValue).Name;
          case FieldType.Message:
          case FieldType.Group:
            return TypeName + ".DefaultInstance";
          default:
            throw new InvalidOperationException("Invalid field descriptor type");
        }
      }
    }

    /// <summary>
    /// Usually the same as CapitalizedName, except when the enclosing type has the same name,
    /// in which case an underscore is appended.
    /// </summary>
    protected string PropertyName {
      get {
        string ret = CapitalizedName;
        if (ret == Descriptor.ContainingType.Name) {
          ret += "_";
        }
        return ret;
      }
    }

    protected string CapitalizedName {
      get { return NameHelpers.UnderscoresToPascalCase(GetFieldName(Descriptor)); }
    }

    protected string Name {
      get { return NameHelpers.UnderscoresToCamelCase(GetFieldName(Descriptor)); }
    }

    protected int Number {
      get { return Descriptor.FieldNumber; }
    }

    protected string TypeName {
      get {
        switch (Descriptor.FieldType) {
          case FieldType.Enum:
            return GetClassName(Descriptor.EnumType);
          case FieldType.Message:
          case FieldType.Group:
            return GetClassName(Descriptor.MessageType);
          default:
            return DescriptorUtil.GetMappedTypeName(Descriptor.MappedType);
        }
      }
    }

    protected string MessageOrGroup {
      get { return Descriptor.FieldType == FieldType.Group ? "Group" : "Message"; }
    }

    /// <summary>
    /// Returns the type name as used in CodedInputStream method names: SFixed32, UInt32 etc.
    /// </summary>
    protected string CapitalizedTypeName {
      get {
        // Our enum names match perfectly. How serendipitous.
        return Descriptor.FieldType.ToString();
      }
    }
  }
}
