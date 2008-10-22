using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using NUnit.Framework;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.ProtoGen;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Tests for the dependency resolution in Generator.
  /// </summary>
  [TestFixture]
  public class DependencyResolutionTest {

    [Test]
    public void TwoDistinctFiles() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name="First" }.Build();
      FileDescriptorProto second = new FileDescriptorProto.Builder { Name="Second" }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first, second } };

      IList<FileDescriptor> converted = Generator.ConvertDescriptors(set);
      Assert.AreEqual(2, converted.Count);
      Assert.AreEqual("First", converted[0].Name);
      Assert.AreEqual(0, converted[0].Dependencies.Count);
      Assert.AreEqual("Second", converted[1].Name);
      Assert.AreEqual(0, converted[1].Dependencies.Count);
    }

    [Test]
    public void FirstDependsOnSecond() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name = "First", DependencyList = {"Second"} }.Build();
      FileDescriptorProto second = new FileDescriptorProto.Builder { Name = "Second" }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first, second } };
      IList<FileDescriptor> converted = Generator.ConvertDescriptors(set);
      Assert.AreEqual(2, converted.Count);
      Assert.AreEqual("First", converted[0].Name);
      Assert.AreEqual(1, converted[0].Dependencies.Count);
      Assert.AreEqual(converted[1], converted[0].Dependencies[0]);
      Assert.AreEqual("Second", converted[1].Name);
      Assert.AreEqual(0, converted[1].Dependencies.Count);
    }

    [Test]
    public void SecondDependsOnFirst() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name = "First" }.Build();
      FileDescriptorProto second = new FileDescriptorProto.Builder { Name = "Second", DependencyList = {"First"} }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first, second } };
      IList<FileDescriptor> converted = Generator.ConvertDescriptors(set);
      Assert.AreEqual(2, converted.Count);
      Assert.AreEqual("First", converted[0].Name);
      Assert.AreEqual(0, converted[0].Dependencies.Count);
      Assert.AreEqual("Second", converted[1].Name);
      Assert.AreEqual(1, converted[1].Dependencies.Count);
      Assert.AreEqual(converted[0], converted[1].Dependencies[0]);
    }

    [Test]
    public void CircularDependency() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name = "First", DependencyList = { "Second" } }.Build();
      FileDescriptorProto second = new FileDescriptorProto.Builder { Name = "Second", DependencyList = { "First" } }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first, second } };
      try {
        Generator.ConvertDescriptors(set);
        Assert.Fail("Expected exception");
      } catch (DependencyResolutionException) {
        // Expected
      }
    }

    [Test]
    public void MissingDependency() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name = "First", DependencyList = { "Second" } }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first } };
      try {
        Generator.ConvertDescriptors(set);
        Assert.Fail("Expected exception");
      } catch (DependencyResolutionException) {
        // Expected
      }
    }

    [Test]
    public void SelfDependency() {
      FileDescriptorProto first = new FileDescriptorProto.Builder { Name = "First", DependencyList = { "First" } }.Build();
      FileDescriptorSet set = new FileDescriptorSet { FileList = { first } };
      try {
        Generator.ConvertDescriptors(set);
        Assert.Fail("Expected exception");
      } catch (DependencyResolutionException) {
        // Expected
      }
    }
  }
}