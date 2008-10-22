using System;
using System.Collections.Generic;
using System.Text;
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
        writer.WriteLine("{0} abstract void {1}(", ClassAccessLevel, Helpers.UnderscoresToPascalCase(method.Name));
        writer.WriteLine("    pb::IRpcController controller,");
        writer.WriteLine("    {0} request,", DescriptorUtil.GetClassName(method.InputType));
        writer.WriteLine("    global::System.Action<{0}> done);", DescriptorUtil.GetClassName(method.OutputType));
      }

      // Generate Descriptor and DescriptorForType.
      writer.WriteLine();
      writer.WriteLine("{0} static pbd::ServiceDescriptor Descriptor {{", ClassAccessLevel);
      writer.WriteLine("  get {{ return {0}.Descriptor.Services[{1}]; }}",
          DescriptorUtil.GetUmbrellaClassName(Descriptor.File), Descriptor.Index);
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
            Helpers.UnderscoresToPascalCase(method.Name), DescriptorUtil.GetClassName(method.InputType));
        writer.WriteLine("      pb::RpcUtil.SpecializeCallback<{0}>(", DescriptorUtil.GetClassName(method.OutputType));
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
          DescriptorUtil.GetClassName(which == RequestOrResponse.Request ? method.InputType : method.OutputType));
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
      writer.WriteLine("{0} class Stub : {1} {{", ClassAccessLevel, DescriptorUtil.GetClassName(Descriptor));
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
        writer.WriteLine("public override void {0}(", Helpers.UnderscoresToPascalCase(method.Name));
        writer.WriteLine("    pb::IRpcController controller,");
        writer.WriteLine("    {0} request,", DescriptorUtil.GetClassName(method.InputType));
        writer.WriteLine("    global::System.Action<{0}> done) {{", DescriptorUtil.GetClassName(method.OutputType));
        writer.Indent();
        writer.WriteLine("channel.CallMethod(Descriptor.Methods[{0}],", method.Index);
        writer.WriteLine("    controller, request, {0}.DefaultInstance,", DescriptorUtil.GetClassName(method.OutputType));
        writer.WriteLine("    pb::RpcUtil.GeneralizeCallback<{0}, {0}.Builder>(done, {0}.DefaultInstance));",
            DescriptorUtil.GetClassName(method.OutputType));
        writer.Outdent();
        writer.WriteLine("}");
      }
      writer.Outdent();
      writer.WriteLine("}");
    }
  }
}
