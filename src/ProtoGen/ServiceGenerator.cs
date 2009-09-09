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

namespace Google.ProtocolBuffers.ProtoGen {
  internal class ServiceGenerator : SourceGeneratorBase<ServiceDescriptor>, ISourceGenerator {

    private enum RequestOrResponse {
      Request,
      Response
    }

    internal ServiceGenerator(ServiceDescriptor descriptor)
      : base(descriptor) {
    }

    public void Generate(TextGenerator writer) {
      writer.WriteLine("{0} abstract class {1} : pb::IService {{", ClassAccessLevel, Descriptor.Name);
      writer.Indent();

      foreach (MethodDescriptor method in Descriptor.Methods) {
        writer.WriteLine("{0} abstract void {1}(", ClassAccessLevel, NameHelpers.UnderscoresToPascalCase(method.Name));
        writer.WriteLine("    pb::IRpcController controller,");
        writer.WriteLine("    {0} request,", GetClassName(method.InputType));
        writer.WriteLine("    global::System.Action<{0}> done);", GetClassName(method.OutputType));
      }

      // Generate Descriptor and DescriptorForType.
      writer.WriteLine();
      writer.WriteLine("{0} static pbd::ServiceDescriptor Descriptor {{", ClassAccessLevel);
      writer.WriteLine("  get {{ return {0}.Descriptor.Services[{1}]; }}",
        Descriptor.File.CSharpOptions.UmbrellaClassname, Descriptor.Index);
      writer.WriteLine("}");
      writer.WriteLine("{0} pbd::ServiceDescriptor DescriptorForType {{", ClassAccessLevel);
      writer.WriteLine("  get { return Descriptor; }");
      writer.WriteLine("}");

      GenerateCallMethod(writer);
      GenerateGetPrototype(RequestOrResponse.Request, writer);
      GenerateGetPrototype(RequestOrResponse.Response, writer);
      GenerateStub(writer);

      writer.Outdent();
      writer.WriteLine("}");
    }

    private void GenerateCallMethod(TextGenerator writer) {
      writer.WriteLine();
      writer.WriteLine("public void CallMethod(", ClassAccessLevel);
      writer.WriteLine("    pbd::MethodDescriptor method,");
      writer.WriteLine("    pb::IRpcController controller,");
      writer.WriteLine("    pb::IMessage request,");
      writer.WriteLine("    global::System.Action<pb::IMessage> done) {");
      writer.Indent();
      writer.WriteLine("if (method.Service != Descriptor) {");
      writer.WriteLine("  throw new global::System.ArgumentException(");
      writer.WriteLine("      \"Service.CallMethod() given method descriptor for wrong service type.\");");
      writer.WriteLine("}");
      writer.WriteLine("switch(method.Index) {");
      writer.Indent();
      foreach (MethodDescriptor method in Descriptor.Methods) {
        writer.WriteLine("case {0}:", method.Index);
        writer.WriteLine("  this.{0}(controller, ({1}) request,",
            NameHelpers.UnderscoresToPascalCase(method.Name), GetClassName(method.InputType));
        writer.WriteLine("      pb::RpcUtil.SpecializeCallback<{0}>(", GetClassName(method.OutputType));
        writer.WriteLine("      done));");
        writer.WriteLine("  return;");
      }
      writer.WriteLine("default:");
      writer.WriteLine("  throw new global::System.InvalidOperationException(\"Can't get here.\");");
      writer.Outdent();
      writer.WriteLine("}");
      writer.Outdent();
      writer.WriteLine("}");
      writer.WriteLine();
    }

    private void GenerateGetPrototype(RequestOrResponse which, TextGenerator writer) {
      writer.WriteLine("public pb::IMessage Get{0}Prototype(pbd::MethodDescriptor method) {{", which);
      writer.Indent();
      writer.WriteLine("if (method.Service != Descriptor) {");
      writer.WriteLine("  throw new global::System.ArgumentException(");
      writer.WriteLine("      \"Service.Get{0}Prototype() given method descriptor for wrong service type.\");", which);
      writer.WriteLine("}");
      writer.WriteLine("switch(method.Index) {");
      writer.Indent();

      foreach (MethodDescriptor method in Descriptor.Methods) {
        writer.WriteLine("case {0}:", method.Index);
        writer.WriteLine("  return {0}.DefaultInstance;", 
          GetClassName(which == RequestOrResponse.Request ? method.InputType : method.OutputType));
      }
      writer.WriteLine("default:");
      writer.WriteLine("  throw new global::System.InvalidOperationException(\"Can't get here.\");");
      writer.Outdent();
      writer.WriteLine("}");
      writer.Outdent();
      writer.WriteLine("}");
      writer.WriteLine();
    }

    private void GenerateStub(TextGenerator writer) {
      writer.WriteLine("public static Stub CreateStub(pb::IRpcChannel channel) {");
      writer.WriteLine("  return new Stub(channel);");
      writer.WriteLine("}");
      writer.WriteLine();
      writer.WriteLine("{0} class Stub : {1} {{", ClassAccessLevel, GetClassName(Descriptor));
      writer.Indent();
      writer.WriteLine("internal Stub(pb::IRpcChannel channel) {");
      writer.WriteLine("  this.channel = channel;");
      writer.WriteLine("}");
      writer.WriteLine();
      writer.WriteLine("private readonly pb::IRpcChannel channel;");
      writer.WriteLine();
      writer.WriteLine("public pb::IRpcChannel Channel {");
      writer.WriteLine("  get { return channel; }");
      writer.WriteLine("}");

      foreach (MethodDescriptor method in Descriptor.Methods) {
        writer.WriteLine();
        writer.WriteLine("public override void {0}(", NameHelpers.UnderscoresToPascalCase(method.Name));
        writer.WriteLine("    pb::IRpcController controller,");
        writer.WriteLine("    {0} request,", GetClassName(method.InputType));
        writer.WriteLine("    global::System.Action<{0}> done) {{", GetClassName(method.OutputType));
        writer.Indent();
        writer.WriteLine("channel.CallMethod(Descriptor.Methods[{0}],", method.Index);
        writer.WriteLine("    controller, request, {0}.DefaultInstance,", GetClassName(method.OutputType));
        writer.WriteLine("    pb::RpcUtil.GeneralizeCallback<{0}, {0}.Builder>(done, {0}.DefaultInstance));",
            GetClassName(method.OutputType));
        writer.Outdent();
        writer.WriteLine("}");
      }
      writer.Outdent();
      writer.WriteLine("}");
    }
  }
}
