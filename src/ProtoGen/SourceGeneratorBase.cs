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

using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal abstract class SourceGeneratorBase<T> where T : IDescriptor {

    private readonly T descriptor;

    protected SourceGeneratorBase(T descriptor) {
      this.descriptor = descriptor;
    }

    protected T Descriptor {
      get { return descriptor; }
    }

    internal static string GetClassName(IDescriptor descriptor) {
      return ToCSharpName(descriptor.FullName, descriptor.File);
    }

    // Groups are hacky:  The name of the field is just the lower-cased name
    // of the group type.  In C#, though, we would like to retain the original
    // capitalization of the type name.
    internal static string GetFieldName(FieldDescriptor descriptor) {
      if (descriptor.FieldType == FieldType.Group) {
        return descriptor.MessageType.Name;
      } else {
        return descriptor.Name;
      }
    }

    internal static string GetFieldConstantName(FieldDescriptor field) {
      return field.CSharpOptions.PropertyName + "FieldNumber";
    }

    private static string ToCSharpName(string name, FileDescriptor file) {
      string result = file.CSharpOptions.Namespace;
      if (file.CSharpOptions.NestClasses) {
        if (result != "") {
          result += ".";
        }
        result += file.CSharpOptions.UmbrellaClassname;
      }
      if (result != "") {
        result += '.';
      }
      string classname;
      if (file.Package == "") {
        classname = name;
      } else {
        // Strip the proto package from full_name since we've replaced it with
        // the C# namespace.
        classname = name.Substring(file.Package.Length + 1);
      }
      result += classname.Replace(".", ".Types.");
      return "global::" + result;
    }

    protected string ClassAccessLevel {
      get { 
        return descriptor.File.CSharpOptions.PublicClasses ? "public" : "internal";
      }
    }

    protected void WriteChildren<TChild>(TextGenerator writer, string region, IEnumerable<TChild> children) 
        where TChild : IDescriptor {
      // Copy the set of children; makes access easier
      List<TChild> copy = new List<TChild>(children);
      if (copy.Count == 0) {
        return;
      }

      if (region != null) {
        writer.WriteLine("#region {0}", region);
      }
      foreach (TChild child in children) {
        SourceGenerators.CreateGenerator(child).Generate(writer);
      }
      if (region != null) {
        writer.WriteLine("#endregion");
        writer.WriteLine();
      }
    }
  }
}
