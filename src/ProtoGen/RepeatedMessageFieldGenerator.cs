using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class RepeatedMessageFieldGenerator : FieldGeneratorBase, IFieldSourceGenerator {

    internal RepeatedMessageFieldGenerator(FieldDescriptor descriptor)
      : base(descriptor) {
    }
    
    public void GenerateMembers(TextGenerator writer) {
      writer.WriteLine("private pbc::PopsicleList<{0}> {1}_ = new pbc::PopsicleList<{0}>();", TypeName, Name);
      writer.WriteLine("public scg::IList<{0}> {1}List {{", TypeName, PropertyName);
      writer.WriteLine("  get {{ return {0}_; }}", Name);
      writer.WriteLine("}");

      // TODO(jonskeet): Redundant API calls? Possibly - include for portability though. Maybe create an option.
      writer.WriteLine("public int {0}Count {{", PropertyName);
      writer.WriteLine("  get {{ return {0}_.Count; }}", Name);
      writer.WriteLine("}");

      writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, PropertyName);
      writer.WriteLine("  return {0}_[index];", Name);
      writer.WriteLine("}");
    }    

    public void GenerateBuilderMembers(TextGenerator writer) {
      // Note:  We can return the original list here, because we make it unmodifiable when we build
      writer.WriteLine("public scg::IList<{0}> {1}List {{", TypeName, PropertyName);
      writer.WriteLine("  get {{ return result.{0}_; }}", Name);
      writer.WriteLine("}");
      writer.WriteLine("public int {0}Count {{", PropertyName);
      writer.WriteLine("  get {{ return result.{0}Count; }}", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, PropertyName);
      writer.WriteLine("  return result.Get{0}(index);", PropertyName);
      writer.WriteLine("}");
      writer.WriteLine("public Builder Set{0}(int index, {1} value) {{", PropertyName, TypeName);
      AddNullCheck(writer);
      writer.WriteLine("  result.{0}_[index] = value;", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      // Extra overload for builder (just on messages)
      writer.WriteLine("public Builder Set{0}(int index, {1}.Builder builderForValue) {{", PropertyName, TypeName);
      AddNullCheck(writer, "builderForValue");
      writer.WriteLine("  result.{0}_[index] = builderForValue.Build();", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder Add{0}({1} value) {{", PropertyName, TypeName);
      AddNullCheck(writer);
      writer.WriteLine("  result.{0}_.Add(value);", Name, TypeName);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      // Extra overload for builder (just on messages)
      writer.WriteLine("public Builder Add{0}({1}.Builder builderForValue) {{", PropertyName, TypeName);
      AddNullCheck(writer, "builderForValue");
      writer.WriteLine("  result.{0}_.Add(builderForValue.Build());", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder AddRange{0}(scg::IEnumerable<{1}> values) {{", PropertyName, TypeName);
      writer.WriteLine("  base.AddRange(values, result.{0}_);", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder Clear{0}() {{", PropertyName);
      writer.WriteLine("  result.{0}_.Clear();", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
    }    

    public void GenerateMergingCode(TextGenerator writer) {
      writer.WriteLine("if (other.{0}_.Count != 0) {{", Name);
      writer.WriteLine("  base.AddRange(other.{0}_, result.{0}_);", Name);
      writer.WriteLine("}");
    }

    public void GenerateBuildingCode(TextGenerator writer) {
      writer.WriteLine("result.{0}_.MakeReadOnly();", Name);
    }

    public void GenerateParsingCode(TextGenerator writer) {
      writer.WriteLine("{0}.Builder subBuilder = {0}.CreateBuilder();", TypeName);
      if (Descriptor.FieldType == FieldType.Group) {
        writer.WriteLine("input.ReadGroup({0}, subBuilder, extensionRegistry);", Number);
      } else {
        writer.WriteLine("input.ReadMessage(subBuilder, extensionRegistry);");
      }
      writer.WriteLine("Add{0}(subBuilder.BuildPartial());", PropertyName);
    }

    public void GenerateSerializationCode(TextGenerator writer) {
      writer.WriteLine("foreach ({0} element in {1}List) {{", TypeName, PropertyName);
      writer.WriteLine("  output.Write{0}({1}, element);", MessageOrGroup, Number);
      writer.WriteLine("}");
    }

    public void GenerateSerializedSizeCode(TextGenerator writer) {
      writer.WriteLine("foreach ({0} element in {1}List) {{", TypeName, PropertyName);
      writer.WriteLine("  size += pb::CodedOutputStream.Compute{0}Size({1}, element);", MessageOrGroup, Number);
      writer.WriteLine("}");
    }
  }
}
