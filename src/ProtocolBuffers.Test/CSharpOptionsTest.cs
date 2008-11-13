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
