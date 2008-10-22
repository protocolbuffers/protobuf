using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class RepeatedEnumFieldGenerator : FieldGeneratorBase, IFieldSourceGenerator {

    internal RepeatedEnumFieldGenerator(FieldDescriptor descriptor)
      : base(descriptor) {
    }

    public void GenerateMembers(TextGenerator writer) {
      writer.WriteLine("private pbc::PopsicleList<{0}> {1}_ = new pbc::PopsicleList<{0}>();", TypeName, Name);
      writer.WriteLine("public scg::IList<{0}> {1}List {{", TypeName, CapitalizedName);
      writer.WriteLine("  get {{ return pbc::Lists.AsReadOnly({0}_); }}", Name);
      writer.WriteLine("}");

      // TODO(jonskeet): Redundant API calls? Possibly - include for portability though. Maybe create an option.
      writer.WriteLine("public int {0}Count {{", CapitalizedName);
      writer.WriteLine("  get {{ return {0}_.Count; }}", Name);
      writer.WriteLine("}");

      writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, CapitalizedName);
      writer.WriteLine("  return {0}_[index];", Name);
      writer.WriteLine("}");
    }

    public void GenerateBuilderMembers(TextGenerator writer) {
      // Note:  We can return the original list here, because we make it unmodifiable when we build
      writer.WriteLine("public scg::IList<{0}> {1}List {{", TypeName, CapitalizedName);
      writer.WriteLine("  get {{ return result.{0}_; }}", Name);
      writer.WriteLine("}");
      writer.WriteLine("public int {0}Count {{", CapitalizedName);
      writer.WriteLine("  get {{ return result.{0}Count; }}", CapitalizedName);
      writer.WriteLine("}");
      writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, CapitalizedName);
      writer.WriteLine("  return result.Get{0}(index);", CapitalizedName);
      writer.WriteLine("}");
      writer.WriteLine("public Builder Set{0}(int index, {1} value) {{", CapitalizedName, TypeName);
      writer.WriteLine("  result.{0}_[index] = value;", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder Add{0}({1} value) {{", CapitalizedName, TypeName);
      writer.WriteLine("  result.{0}_.Add(value);", Name, TypeName);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder AddRange{0}(scg::IEnumerable<{1}> values) {{", CapitalizedName, TypeName);
      writer.WriteLine("  base.AddRange(values, result.{0}_);", Name);
      writer.WriteLine("  return this;");
      writer.WriteLine("}");
      writer.WriteLine("public Builder Clear{0}() {{", CapitalizedName);
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
      // TODO(jonskeet): Make a more efficient way of doing this
      writer.WriteLine("int rawValue = input.ReadEnum();");
      writer.WriteLine("if (!global::System.Enum.IsDefined(typeof({0}), rawValue)) {{", TypeName);
      writer.WriteLine("  unknownFields.MergeVarintField({0}, (ulong) rawValue);", Number);
      writer.WriteLine("} else {");
      writer.WriteLine("  Add{0}(({1}) rawValue);", CapitalizedName, TypeName);
      writer.WriteLine("}");
    }

    public void GenerateSerializationCode(TextGenerator writer) {
      writer.WriteLine("foreach ({0} element in {1}List) {{", TypeName, CapitalizedName);
      writer.WriteLine("  output.WriteEnum({0}, (int) element);", Number);
      writer.WriteLine("}");
    }

    public void GenerateSerializedSizeCode(TextGenerator writer) {
      writer.WriteLine("foreach ({0} element in {1}List) {{", TypeName, CapitalizedName);
      writer.WriteLine("  size += pb::CodedOutputStream.ComputeEnumSize({0}, (int) element);", Number);
      writer.WriteLine("}");
    }
  }
}
