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

    internal static string GetFullUmbrellaClassName(IDescriptor descriptor) {
      CSharpFileOptions options = descriptor.File.CSharpOptions;
      string result = options.Namespace;
      if (result != "") result += '.';
      result += options.UmbrellaClassname;
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
