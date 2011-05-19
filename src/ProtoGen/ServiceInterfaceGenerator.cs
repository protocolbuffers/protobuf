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

using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class ServiceGenerator : SourceGeneratorBase<ServiceDescriptor>, ISourceGenerator {

    readonly CSharpServiceType svcType;
    ISourceGenerator _generator;

    internal ServiceGenerator(ServiceDescriptor descriptor)
      : base(descriptor) {
      svcType = descriptor.File.CSharpOptions.ServiceGeneratorType;
      switch (svcType) {
        case CSharpServiceType.NONE:
          _generator = new NoServicesGenerator(descriptor);
          break;
        case CSharpServiceType.GENERIC:
          _generator = new GenericServiceGenerator(descriptor);
          break;
        case CSharpServiceType.INTERFACE:
          _generator = new ServiceInterfaceGenerator(descriptor);
          break;
        case CSharpServiceType.IRPCDISPATCH:
          _generator = new RpcServiceGenerator(descriptor);
          break;
        default: throw new ApplicationException("Unknown ServiceGeneratorType = " + svcType.ToString());
      }
    }

    public void Generate(TextGenerator writer) {
      _generator.Generate(writer);
    }
      
    class NoServicesGenerator : SourceGeneratorBase<ServiceDescriptor>, ISourceGenerator {
      
      public NoServicesGenerator(ServiceDescriptor descriptor)
        : base(descriptor) {
      }
      public virtual void Generate(TextGenerator writer) {
        writer.WriteLine("/*");
        writer.WriteLine("* Service generation is now disabled by default, use the following option to enable:");
        writer.WriteLine("* option (google.protobuf.csharp_file_options).service_generator_type = GENERIC;");
        writer.WriteLine("*/");
      }
    }

    class ServiceInterfaceGenerator : SourceGeneratorBase<ServiceDescriptor>, ISourceGenerator {
      
      public ServiceInterfaceGenerator(ServiceDescriptor descriptor)
        : base(descriptor) {
      }

      public virtual void Generate(TextGenerator writer) {

        CSharpServiceOptions options = Descriptor.Options.GetExtension(CSharpOptions.CsharpServiceOptions);
        if (options != null && options.HasInterfaceId) {
          writer.WriteLine("[global::System.Runtime.InteropServices.GuidAttribute(\"{0}\")]", new Guid(options.InterfaceId));
        }
        writer.WriteLine("{0} partial interface I{1} {{", ClassAccessLevel, Descriptor.Name);
        writer.Indent();

        foreach (MethodDescriptor method in Descriptor.Methods)
        {
          CSharpMethodOptions mth = method.Options.GetExtension(CSharpOptions.CsharpMethodOptions);
          if(mth.HasDispatchId) {
            writer.WriteLine("[global::System.Runtime.InteropServices.DispId({0})]", mth.DispatchId);
          }
          writer.WriteLine("{0} {1}({2} {3});", GetClassName(method.OutputType), NameHelpers.UnderscoresToPascalCase(method.Name), GetClassName(method.InputType), NameHelpers.UnderscoresToCamelCase(method.InputType.Name));
        }

        writer.Outdent();
        writer.WriteLine("}");
      }
    }
    
    class RpcServiceGenerator : ServiceInterfaceGenerator {

      public RpcServiceGenerator(ServiceDescriptor descriptor)
        : base(descriptor) {
      }

      public override void Generate(TextGenerator writer)
      {
        base.Generate(writer);

        writer.WriteLine();

        // CLIENT Proxy
        {
          if (Descriptor.File.CSharpOptions.ClsCompliance)
            writer.WriteLine("[global::System.CLSCompliant(false)]");
          writer.WriteLine("{0} partial class {1} : I{1}, pb::IRpcDispatch, global::System.IDisposable {{", ClassAccessLevel, Descriptor.Name);
          writer.Indent();
          writer.WriteLine("private readonly bool dispose;");
          writer.WriteLine("private readonly pb::IRpcDispatch dispatch;");

          writer.WriteLine("public {0}(pb::IRpcDispatch dispatch) : this(dispatch, true) {{", Descriptor.Name);
          writer.WriteLine("}");
          writer.WriteLine("public {0}(pb::IRpcDispatch dispatch, bool dispose) {{", Descriptor.Name);
          writer.WriteLine("  if (null == (this.dispatch = dispatch)) throw new global::System.ArgumentNullException();");
          writer.WriteLine("  this.dispose = dispose && dispatch is global::System.IDisposable;");
          writer.WriteLine("}");
          writer.WriteLine();

          writer.WriteLine("public void Dispose() {");
          writer.WriteLine("  if (dispose) ((global::System.IDisposable)dispatch).Dispose();");
          writer.WriteLine("}");
          writer.WriteLine();
          writer.WriteLine("TMessage pb::IRpcDispatch.CallMethod<TMessage, TBuilder>(string method, pb::IMessageLite request, pb::IBuilderLite<TMessage, TBuilder> response) {");
          writer.WriteLine("  return dispatch.CallMethod(method, request, response);");
          writer.WriteLine("}");
          writer.WriteLine();

          foreach (MethodDescriptor method in Descriptor.Methods) {
            writer.WriteLine("public {0} {1}({2} {3}) {{", GetClassName(method.OutputType), NameHelpers.UnderscoresToPascalCase(method.Name), GetClassName(method.InputType), NameHelpers.UnderscoresToCamelCase(method.InputType.Name));
            writer.WriteLine("   return dispatch.CallMethod(\"{0}\", {1}, {2}.CreateBuilder());",
                             method.Name,
                             NameHelpers.UnderscoresToCamelCase(method.InputType.Name),
                             GetClassName(method.OutputType)
              );
            writer.WriteLine("}");
            writer.WriteLine();
          }
        }
        // SERVER - DISPATCH
        {
          if (Descriptor.File.CSharpOptions.ClsCompliance)
            writer.WriteLine("[global::System.CLSCompliant(false)]");
          writer.WriteLine("public partial class Dispatch : pb::IRpcDispatch, global::System.IDisposable {");
          writer.Indent();
          writer.WriteLine("private readonly bool dispose;");
          writer.WriteLine("private readonly I{0} implementation;", Descriptor.Name);

          writer.WriteLine("public Dispatch(I{0} implementation) : this(implementation, true) {{", Descriptor.Name);
          writer.WriteLine("}");
          writer.WriteLine("public Dispatch(I{0} implementation, bool dispose) {{", Descriptor.Name);
          writer.WriteLine("  if (null == (this.implementation = implementation)) throw new global::System.ArgumentNullException();");
          writer.WriteLine("  this.dispose = dispose && implementation is global::System.IDisposable;");
          writer.WriteLine("}");
          writer.WriteLine();

          writer.WriteLine("public void Dispose() {");
          writer.WriteLine("  if (dispose) ((global::System.IDisposable)implementation).Dispose();");
          writer.WriteLine("}");
          writer.WriteLine();

          writer.WriteLine("public TMessage CallMethod<TMessage, TBuilder>(string methodName, pb::IMessageLite request, pb::IBuilderLite<TMessage, TBuilder> response)");
          writer.WriteLine("  where TMessage : IMessageLite<TMessage, TBuilder>");
          writer.WriteLine("  where TBuilder : IBuilderLite<TMessage, TBuilder> {");
          writer.Indent();
          writer.WriteLine("switch(methodName) {");
          writer.Indent();

          foreach (MethodDescriptor method in Descriptor.Methods) {
            writer.WriteLine("case \"{0}\": return response.MergeFrom(implementation.{1}(({2})request)).Build();", method.Name, NameHelpers.UnderscoresToPascalCase(method.Name), GetClassName(method.InputType));
          }
          writer.WriteLine("default: throw new global::System.MissingMethodException(typeof(ISearchService).FullName, methodName);");
          writer.Outdent();
          writer.WriteLine("}");//end switch
          writer.Outdent();
          writer.WriteLine("}");//end invoke
          writer.Outdent();
          writer.WriteLine("}");//end server
        }
        // SERVER - STUB
        {
          if (Descriptor.File.CSharpOptions.ClsCompliance)
            writer.WriteLine("[global::System.CLSCompliant(false)]");
          writer.WriteLine("public partial class ServerStub : pb::IRpcServerStub, global::System.IDisposable {");
          writer.Indent();
          writer.WriteLine("private readonly bool dispose;");
          writer.WriteLine("private readonly pb::IRpcDispatch implementation;", Descriptor.Name);

          writer.WriteLine("public ServerStub(I{0} implementation) : this(implementation, true) {{", Descriptor.Name);
          writer.WriteLine("}");
          writer.WriteLine("public ServerStub(I{0} implementation, bool dispose) : this(new Dispatch(implementation, dispose), dispose) {{", Descriptor.Name);
          writer.WriteLine("}");

          writer.WriteLine("public ServerStub(pb::IRpcDispatch implementation) : this(implementation, true) {");
          writer.WriteLine("}");
          writer.WriteLine("public ServerStub(pb::IRpcDispatch implementation, bool dispose) {");
          writer.WriteLine("  if (null == (this.implementation = implementation)) throw new global::System.ArgumentNullException();");
          writer.WriteLine("  this.dispose = dispose && implementation is global::System.IDisposable;");
          writer.WriteLine("}");
          writer.WriteLine();

          writer.WriteLine("public void Dispose() {");
          writer.WriteLine("  if (dispose) ((global::System.IDisposable)implementation).Dispose();");
          writer.WriteLine("}");
          writer.WriteLine();

          writer.WriteLine("public pb::IMessageLite CallMethod(string methodName, pb::CodedInputStream input, pb::ExtensionRegistry registry) {{", Descriptor.Name);
          writer.Indent();
          writer.WriteLine("switch(methodName) {");
          writer.Indent();

          foreach (MethodDescriptor method in Descriptor.Methods)
          {
            writer.WriteLine("case \"{0}\": return implementation.CallMethod(methodName, {1}.ParseFrom(input, registry), {2}.CreateBuilder());", method.Name, GetClassName(method.InputType), GetClassName(method.OutputType));
          }
          writer.WriteLine("default: throw new global::System.MissingMethodException(typeof(ISearchService).FullName, methodName);");
          writer.Outdent();
          writer.WriteLine("}");//end switch
          writer.Outdent();
          writer.WriteLine("}");//end invoke
          writer.Outdent();
          writer.WriteLine("}");//end server
        }

        writer.Outdent();
        writer.WriteLine("}");
      }
    }
  }
}
