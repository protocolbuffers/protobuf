using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Utility class for determining namespaces etc.
  /// </summary>
  internal static class DescriptorUtil {

    internal static bool NestClasses(IDescriptor descriptor) {
      // Defaults to false
      return descriptor.File.Options.GetExtension(CSharpOptions.CSharpNestClasses);
    }

    internal static string GetNamespace(FileDescriptor descriptor) {
      if (descriptor.Name == "google/protobuf/descriptor.proto") {
        return typeof(DescriptorProtoFile).Namespace;
      }
      return descriptor.Options.HasExtension(CSharpOptions.CSharpNamespace) ?
          descriptor.Options.GetExtension(CSharpOptions.CSharpNamespace) : descriptor.Package;
    }

    // Groups are hacky:  The name of the field is just the lower-cased name
    // of the group type.  In C#, though, we would like to retain the original
    // capitalization of the type name.
    internal static string GetFieldName(FieldDescriptor descriptor) {
      if (descriptor.FieldType == FieldType.Group) {
        return descriptor.MessageType.Name;
      } else {
        return descriptor.Name;
      }
    }

    internal static string GetClassName(IDescriptor descriptor) {
      return ToCSharpName(descriptor.FullName, descriptor.File);
    }

    internal static string GetFullUmbrellaClassName(FileDescriptor descriptor) {
      string result = GetNamespace(descriptor);
      if (result != "") result += '.';
      result += GetUmbrellaClassName(descriptor);
      return "global::" + result;
    }

    internal static string GetUmbrellaClassName(FileDescriptor descriptor) {
      if (descriptor.Name == "google/protobuf/descriptor.proto") {
        return typeof(DescriptorProtoFile).Name;
      }
      FileOptions options = descriptor.Options;
      if (options.HasExtension(CSharpOptions.CSharpUmbrellaClassname)) {
        return descriptor.Options.GetExtension(CSharpOptions.CSharpUmbrellaClassname);
      }
      int lastSlash = descriptor.Name.LastIndexOf('/');
      string baseName = descriptor.Name.Substring(lastSlash + 1);
      return Helpers.UnderscoresToPascalCase(StripProto(baseName));
    }

    private static string StripProto(string text) {
      if (!Helpers.StripSuffix(ref text, ".protodevel")) {
        Helpers.StripSuffix(ref text, ".proto");
      }
      return text;
    }

    private static string ToCSharpName(string name, FileDescriptor file) {
      string result;
      if (!NestClasses(file)) {
        result = GetNamespace(file);
      } else {
        result = GetUmbrellaClassName(file);
      }
      if (result != "") {
        result += '.';
      }
      string classname;
      if (file.Package == "") {
        classname = name;
      } else {
        // Strip the proto package from full_name since we've replaced it with
        // the C# namespace.
        classname = name.Substring(file.Package.Length + 1);
      }
      result += classname.Replace(".", ".Types.");
      return "global::" + result;
    }

    internal static string GetMappedTypeName(MappedType type) {
      switch(type) {
        case MappedType.Int32:      return "int";
        case MappedType.Int64:      return "long";
        case MappedType.UInt32:     return "uint";
        case MappedType.UInt64:     return "ulong";
        case MappedType.Single:     return "float";
        case MappedType.Double:     return "double";
        case MappedType.Boolean:    return "bool";
        case MappedType.String:     return "string";
        case MappedType.ByteString: return "pb::ByteString";
        case MappedType.Enum:       return null;
        case MappedType.Message:    return null;
        default:
          throw new ArgumentOutOfRangeException("Unknown mapped type " + type);
      }
    }
  }
}
