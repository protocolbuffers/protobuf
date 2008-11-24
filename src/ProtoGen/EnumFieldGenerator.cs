using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class EnumFieldGenerator : FieldGeneratorBase, IFieldSourceGenerator {
    internal EnumFieldGenerator(FieldDescriptor descriptor)
        : base(descriptor) {
    }

    public void GenerateMembers(TextGenerator writer) {
      writer.WriteLine("private bool has{0};", PropertyName);
      writer.WriteLine("private {0} {1}_ = {2};", TypeName, Name, DefaultValue);
      writer.WriteLine("public bool Has{0} {{", PropertyName);
      writer.WriteLine("  get {{ return has{0}; }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public {0} {1} {{", TypeName, PropertyName);
      writer.WriteLine("  get {{ return {0}_; }}", Name);
      writer.WriteLine("}");
    }

    public void GenerateBuilderMembers(TextGenerator writer) {
      writer.WriteLine("public bool Has{0} {{", PropertyName);
      writer.WriteLine(" get {{ return result.Has{0}; }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public {0} {1} {{", TypeName, PropertyName);
      writer.WriteLine("  get {{ return result.{0}; }}", PropertyName);
      writer.WriteLine("  set {{ Set{0}(value); }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public Builder Set{0}({1} value) {{", PropertyName, TypeName);
      writer.WriteLine("  result.has{0} = true;", PropertyName);
      writer.WriteLine("  result.{0}_ = value;", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder Clear{0}() {{", PropertyName);
      writer.WriteLine("  result.has{0} = false;", PropertyName);
      writer.WriteLine("  result.{0}_ = {1};", Name, DefaultValue);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
    }

    public void GenerateMergingCode(TextGenerator writer) {
      writer.WriteLine("if (other.Has{0}) {{", PropertyName);
      writer.WriteLine("  {0} = other.{0};", PropertyName);
      writer.WriteLine("}");
    }

    public void GenerateBuildingCode(TextGenerator writer) {
      // Nothing to do here for enum types
    }

    public void GenerateParsingCode(TextGenerator writer) {
      // TODO(jonskeet): Make a more efficient way of doing this
      writer.WriteLine("int rawValue = input.ReadEnum();");
      writer.WriteLine("if (!global::System.Enum.IsDefined(typeof({0}), rawValue)) {{", TypeName);
      writer.WriteLine("  unknownFields.MergeVarintField({0}, (ulong) rawValue);", Number);
      writer.WriteLine("} else {");
      writer.WriteLine("  {0} = ({1}) rawValue;", PropertyName, TypeName);
      writer.WriteLine("}");
    }

    public void GenerateSerializationCode(TextGenerator writer) {
      writer.WriteLine("if (Has{0}) {{", PropertyName);
      writer.WriteLine("  output.WriteEnum({0}, (int) {1});", Number, PropertyName);
      writer.WriteLine("}");
    }

    public void GenerateSerializedSizeCode(TextGenerator writer) {
      writer.WriteLine("if (Has{0}) {{", PropertyName);
      writer.WriteLine("  size += pb::CodedOutputStream.ComputeEnumSize({0}, (int) {1});", Number, PropertyName);
      writer.WriteLine("}");    
    }
  }
}
