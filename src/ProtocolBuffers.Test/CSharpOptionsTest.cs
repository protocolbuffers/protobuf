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

using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class DescriptorUtilTest {
    [Test]
    public void ExplicitNamespace() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder {
        Name = "x", Package = "pack", Options = new FileOptions.Builder().SetExtension(CSharpOptions.CSharpFileOptions,
          new CSharpFileOptions.Builder { Namespace = "Foo.Bar" }.Build()).Build()
      }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("Foo.Bar", descriptor.CSharpOptions.Namespace);
    }

    [Test]
    public void NoNamespaceFallsBackToPackage() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x", Package = "pack" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("pack", descriptor.CSharpOptions.Namespace);
    }

    [Test]
    public void NoNamespaceOrPackageFallsBackToEmptyString() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("", descriptor.CSharpOptions.Namespace);
    }

    [Test]
    public void ExplicitlyNamedFileClass() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder {
        Name = "x", Options = new FileOptions.Builder().SetExtension(CSharpOptions.CSharpFileOptions,
          new CSharpFileOptions.Builder { UmbrellaClassname = "Foo" }.Build()).Build()
      }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("Foo", descriptor.CSharpOptions.UmbrellaClassname);
    }

    [Test]
    public void ImplicitFileClassWithProtoSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar.proto" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", descriptor.CSharpOptions.UmbrellaClassname);
    }

    [Test]
    public void ImplicitFileClassWithProtoDevelSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar.protodevel" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", descriptor.CSharpOptions.UmbrellaClassname);
    }

    [Test]
    public void ImplicitFileClassWithNoSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", descriptor.CSharpOptions.UmbrellaClassname);
    }

    [Test]
    public void ImplicitFileClassWithDirectoryStructure() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x/y/foo_bar" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", descriptor.CSharpOptions.UmbrellaClassname);
    }
  }
}
