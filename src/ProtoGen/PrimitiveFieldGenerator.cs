using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  // TODO(jonskeet): Refactor this. There's loads of common code here.
  internal class PrimitiveFieldGenerator : FieldGeneratorBase, IFieldSourceGenerator {

    internal PrimitiveFieldGenerator(FieldDescriptor descriptor)
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
      writer.WriteLine("  get {{ return result.Has{0}; }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public {0} {1} {{", TypeName, PropertyName);
      writer.WriteLine("  get {{ return result.{0}; }}", PropertyName);
      writer.WriteLine("  set {{ Set{0}(value); }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public Builder Set{0}({1} value) {{", PropertyName, TypeName);
      AddNullCheck(writer);
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
      // Nothing to do here for primitive types
    }

    public void GenerateParsingCode(TextGenerator writer) {
      writer.WriteLine("{0} = input.Read{1}();", PropertyName, CapitalizedTypeName);
    }

    public void GenerateSerializationCode(TextGenerator writer) {
      writer.WriteLine("if (Has{0}) {{", PropertyName);
      writer.WriteLine("  output.Write{0}({1}, {2});", CapitalizedTypeName, Number, PropertyName);
      writer.WriteLine("}");
    }

    public void GenerateSerializedSizeCode(TextGenerator writer) {
      writer.WriteLine("if (Has{0}) {{", PropertyName);
      writer.WriteLine("  size += pb::CodedOutputStream.Compute{0}Size({1}, {2});",
          CapitalizedTypeName, Number, PropertyName);
      writer.WriteLine("}");
    }
  }
}
