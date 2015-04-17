#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#endregion

using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen
{
    internal class RepeatedMessageFieldGenerator : FieldGeneratorBase, IFieldSourceGenerator
    {
        internal RepeatedMessageFieldGenerator(FieldDescriptor descriptor, int fieldOrdinal)
            : base(descriptor, fieldOrdinal)
        {
        }

        public void GenerateMembers(TextGenerator writer)
        {
            writer.WriteLine("private pbc::PopsicleList<{0}> {1}_ = new pbc::PopsicleList<{0}>();", TypeName, Name);
            AddDeprecatedFlag(writer);
            writer.WriteLine("public scg::IList<{0}> {1}List {{", TypeName, PropertyName);
            writer.WriteLine("  get {{ return {0}_; }}", Name);
            writer.WriteLine("}");

            // TODO(jonskeet): Redundant API calls? Possibly - include for portability though. Maybe create an option.
            AddDeprecatedFlag(writer);
            writer.WriteLine("public int {0}Count {{", PropertyName);
            writer.WriteLine("  get {{ return {0}_.Count; }}", Name);
            writer.WriteLine("}");

            AddDeprecatedFlag(writer);
            writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, PropertyName);
            writer.WriteLine("  return {0}_[index];", Name);
            writer.WriteLine("}");
        }

        public void GenerateBuilderMembers(TextGenerator writer)
        {
            // Note:  We can return the original list here, because we make it unmodifiable when we build
            // We return it via IPopsicleList so that collection initializers work more pleasantly.
            AddDeprecatedFlag(writer);
            writer.WriteLine("public pbc::IPopsicleList<{0}> {1}List {{", TypeName, PropertyName);
            writer.WriteLine("  get {{ return PrepareBuilder().{0}_; }}", Name);
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public int {0}Count {{", PropertyName);
            writer.WriteLine("  get {{ return result.{0}Count; }}", PropertyName);
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public {0} Get{1}(int index) {{", TypeName, PropertyName);
            writer.WriteLine("  return result.Get{0}(index);", PropertyName);
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder Set{0}(int index, {1} value) {{", PropertyName, TypeName);
            AddNullCheck(writer);
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_[index] = value;", Name);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
            // Extra overload for builder (just on messages)
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder Set{0}(int index, {1}.Builder builderForValue) {{", PropertyName, TypeName);
            AddNullCheck(writer, "builderForValue");
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_[index] = builderForValue.Build();", Name);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder Add{0}({1} value) {{", PropertyName, TypeName);
            AddNullCheck(writer);
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_.Add(value);", Name, TypeName);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
            // Extra overload for builder (just on messages)
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder Add{0}({1}.Builder builderForValue) {{", PropertyName, TypeName);
            AddNullCheck(writer, "builderForValue");
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_.Add(builderForValue.Build());", Name);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder AddRange{0}(scg::IEnumerable<{1}> values) {{", PropertyName, TypeName);
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_.Add(values);", Name);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
            AddDeprecatedFlag(writer);
            writer.WriteLine("public Builder Clear{0}() {{", PropertyName);
            writer.WriteLine("  PrepareBuilder();");
            writer.WriteLine("  result.{0}_.Clear();", Name);
            writer.WriteLine("  return this;");
            writer.WriteLine("}");
        }

        public void GenerateMergingCode(TextGenerator writer)
        {
            writer.WriteLine("if (other.{0}_.Count != 0) {{", Name);
            writer.WriteLine("  result.{0}_.Add(other.{0}_);", Name);
            writer.WriteLine("}");
        }

        public void GenerateBuildingCode(TextGenerator writer)
        {
            writer.WriteLine("{0}_.MakeReadOnly();", Name);
        }

        public void GenerateParsingCode(TextGenerator writer)
        {
            writer.WriteLine(
                "input.Read{0}Array(tag, field_name, result.{1}_, {2}.DefaultInstance, extensionRegistry);",
                MessageOrGroup, Name, TypeName);
        }

        public void GenerateSerializationCode(TextGenerator writer)
        {
            writer.WriteLine("if ({0}_.Count > 0) {{", Name);
            writer.Indent();
            writer.WriteLine("output.Write{0}Array({1}, field_names[{3}], {2}_);", MessageOrGroup, Number, Name,
                             FieldOrdinal, Descriptor.FieldType);
            writer.Outdent();
            writer.WriteLine("}");
        }

        public void GenerateSerializedSizeCode(TextGenerator writer)
        {
            writer.WriteLine("foreach ({0} element in {1}List) {{", TypeName, PropertyName);
            writer.WriteLine("  size += pb::CodedOutputStream.Compute{0}Size({1}, element);", MessageOrGroup, Number);
            writer.WriteLine("}");
        }

        public override void WriteHash(TextGenerator writer)
        {
            writer.WriteLine("foreach({0} i in {1}_)", TypeName, Name);
            writer.WriteLine("  hash ^= i.GetHashCode();");
        }

        public override void WriteEquals(TextGenerator writer)
        {
            writer.WriteLine("if({0}_.Count != other.{0}_.Count) return false;", Name);
            writer.WriteLine("for(int ix=0; ix < {0}_.Count; ix++)", Name);
            writer.WriteLine("  if(!{0}_[ix].Equals(other.{0}_[ix])) return false;", Name);
        }

        public override void WriteToString(TextGenerator writer)
        {
            writer.WriteLine("PrintField(\"{0}\", {1}_, writer);",
                             Descriptor.FieldType == FieldType.Group ? Descriptor.MessageType.Name : Descriptor.Name,
                             Name);
        }
    }
}