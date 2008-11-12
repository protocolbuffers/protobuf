using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class ExtensionGenerator : SourceGeneratorBase<FieldDescriptor>, ISourceGenerator {
    internal ExtensionGenerator(FieldDescriptor descriptor) : base(descriptor) {
    }

    public void Generate(TextGenerator writer) {
      string name = NameHelpers.UnderscoresToPascalCase(GetFieldName(Descriptor));

      string type;
      switch (Descriptor.MappedType) {
        case MappedType.Message:
          type = GetClassName(Descriptor.MessageType);
          break;
        case MappedType.Enum:
          type = GetClassName(Descriptor.EnumType);
          break;
        default:
          type = DescriptorUtil.GetMappedTypeName(Descriptor.MappedType);
          break;
      }

      if (Descriptor.IsRepeated) {
        writer.WriteLine("{0} static readonly", ClassAccessLevel);
        writer.WriteLine("    pb::GeneratedExtensionBase<scg::IList<{0}>> {1} =", type, name);
        writer.WriteLine("    pb::GeneratedRepeatExtension<{0}>.CreateInstance(Descriptor.Extensions[{1}]);", type, Descriptor.Index);
      } else {
        writer.WriteLine("{0} static readonly pb::GeneratedExtensionBase<{1}> {2} =", ClassAccessLevel, type, name);
        writer.WriteLine("    pb::GeneratedSingleExtension<{0}>.CreateInstance(Descriptor.Extensions[{1}]);", type, Descriptor.Index);
      }
    }
  }
}
