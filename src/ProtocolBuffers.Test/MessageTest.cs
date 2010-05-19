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

using System.IO;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Miscellaneous tests for message operations that apply to both
  /// generated and dynamic messages.
  /// </summary>
  [TestFixture]
  public class MessageTest {
    // =================================================================
    // Message-merging tests.

    private static readonly TestAllTypes MergeSource = new TestAllTypes.Builder { 
        OptionalInt32 = 1, 
        OptionalString = "foo", 
        OptionalForeignMessage = ForeignMessage.DefaultInstance,
    }.AddRepeatedString("bar").Build();

    private static readonly TestAllTypes MergeDest = new TestAllTypes.Builder {
        OptionalInt64 = 2,
        OptionalString = "baz",
        OptionalForeignMessage = new ForeignMessage.Builder { C=3 }.Build(),
    }.AddRepeatedString("qux").Build();

    private const string MergeResultText =
        "optional_int32: 1\n" +
        "optional_int64: 2\n" +
        "optional_string: \"foo\"\n" +
        "optional_foreign_message {\n" +
        "  c: 3\n" +
        "}\n" +
        "repeated_string: \"qux\"\n" +
        "repeated_string: \"bar\"\n";

    [Test]
    public void MergeFrom() {
      TestAllTypes result = TestAllTypes.CreateBuilder(MergeDest).MergeFrom(MergeSource).Build();

      Assert.AreEqual(MergeResultText, result.ToString());
    }

    /// <summary>
    /// Test merging a DynamicMessage into a GeneratedMessage. 
    /// As long as they have the same descriptor, this should work, but it is an
    /// entirely different code path.
    /// </summary>
    [Test]
    public void MergeFromDynamic() {
      TestAllTypes result = (TestAllTypes) TestAllTypes.CreateBuilder(MergeDest)
          .MergeFrom(DynamicMessage.CreateBuilder(MergeSource).Build())
          .Build();

      Assert.AreEqual(MergeResultText, result.ToString());
    }

    /// <summary>
    /// Test merging two DynamicMessages.
    /// </summary>
    [Test]
    public void DynamicMergeFrom() {
      DynamicMessage result = (DynamicMessage) DynamicMessage.CreateBuilder(MergeDest)
          .MergeFrom((DynamicMessage) DynamicMessage.CreateBuilder(MergeSource).Build())
          .Build();

      Assert.AreEqual(MergeResultText, result.ToString());
    }

    // =================================================================
    // Required-field-related tests.

    private static readonly TestRequired TestRequiredUninitialized = TestRequired.DefaultInstance;
    private static readonly TestRequired TestRequiredInitialized = new TestRequired.Builder {
        A = 1, B = 2, C = 3
    }.Build();

    [Test]
    public void Initialization() {
      TestRequired.Builder builder = TestRequired.CreateBuilder();

      Assert.IsFalse(builder.IsInitialized);
      builder.A = 1;
      Assert.IsFalse(builder.IsInitialized);
      builder.B = 1;
      Assert.IsFalse(builder.IsInitialized);
      builder.C = 1;
      Assert.IsTrue(builder.IsInitialized);
    }

    [Test]
    public void RequiredForeign() {
      TestRequiredForeign.Builder builder = TestRequiredForeign.CreateBuilder();

      Assert.IsTrue(builder.IsInitialized);

      builder.SetOptionalMessage(TestRequiredUninitialized);
      Assert.IsFalse(builder.IsInitialized);

      builder.SetOptionalMessage(TestRequiredInitialized);
      Assert.IsTrue(builder.IsInitialized);

      builder.AddRepeatedMessage(TestRequiredUninitialized);
      Assert.IsFalse(builder.IsInitialized);

      builder.SetRepeatedMessage(0, TestRequiredInitialized);
      Assert.IsTrue(builder.IsInitialized);
    }

    [Test]
    public void RequiredExtension() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();

      Assert.IsTrue(builder.IsInitialized);

      builder.SetExtension(TestRequired.Single, TestRequiredUninitialized);
      Assert.IsFalse(builder.IsInitialized);

      builder.SetExtension(TestRequired.Single, TestRequiredInitialized);
      Assert.IsTrue(builder.IsInitialized);

      builder.AddExtension(TestRequired.Multi, TestRequiredUninitialized);
      Assert.IsFalse(builder.IsInitialized);

      builder.SetExtension(TestRequired.Multi, 0, TestRequiredInitialized);
      Assert.IsTrue(builder.IsInitialized);
    }

    [Test]
    public void RequiredDynamic() {
      MessageDescriptor descriptor = TestRequired.Descriptor;
      DynamicMessage.Builder builder = DynamicMessage.CreateBuilder(descriptor);

      Assert.IsFalse(builder.IsInitialized);
      builder[descriptor.FindDescriptor<FieldDescriptor>("a")] = 1;
      Assert.IsFalse(builder.IsInitialized);
      builder[descriptor.FindDescriptor<FieldDescriptor>("b")] = 1;
      Assert.IsFalse(builder.IsInitialized);
      builder[descriptor.FindDescriptor<FieldDescriptor>("c")] = 1;
      Assert.IsTrue(builder.IsInitialized);
    }

    [Test]
    public void RequiredDynamicForeign() {
      MessageDescriptor descriptor = TestRequiredForeign.Descriptor;
      DynamicMessage.Builder builder = DynamicMessage.CreateBuilder(descriptor);

      Assert.IsTrue(builder.IsInitialized);

      builder[descriptor.FindDescriptor<FieldDescriptor>("optional_message")] = TestRequiredUninitialized;
      Assert.IsFalse(builder.IsInitialized);

      builder[descriptor.FindDescriptor<FieldDescriptor>("optional_message")] = TestRequiredInitialized;
      Assert.IsTrue(builder.IsInitialized);

      builder.AddRepeatedField(descriptor.FindDescriptor<FieldDescriptor>("repeated_message"), TestRequiredUninitialized);
      Assert.IsFalse(builder.IsInitialized);

      builder.SetRepeatedField(descriptor.FindDescriptor<FieldDescriptor>("repeated_message"), 0, TestRequiredInitialized);
      Assert.IsTrue(builder.IsInitialized);
    }

    [Test]
    public void UninitializedException() {
      try {
        TestRequired.CreateBuilder().Build();
        Assert.Fail("Should have thrown an exception.");
      } catch (UninitializedMessageException e) {
        Assert.AreEqual("Message missing required fields: a, b, c", e.Message);
      }
    }

    [Test]
    public void BuildPartial() {
      // We're mostly testing that no exception is thrown.
      TestRequired message = TestRequired.CreateBuilder().BuildPartial();
      Assert.IsFalse(message.IsInitialized);
    }

    [Test]
    public void NestedUninitializedException() {
      try {
        TestRequiredForeign.CreateBuilder()
          .SetOptionalMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .Build();
        Assert.Fail("Should have thrown an exception.");
      } catch (UninitializedMessageException e) {
        Assert.AreEqual(
          "Message missing required fields: " +
          "optional_message.a, " +
          "optional_message.b, " +
          "optional_message.c, " +
          "repeated_message[0].a, " +
          "repeated_message[0].b, " +
          "repeated_message[0].c, " +
          "repeated_message[1].a, " +
          "repeated_message[1].b, " +
          "repeated_message[1].c",
          e.Message);
      }
    }

    [Test]
    public void BuildNestedPartial() {
      // We're mostly testing that no exception is thrown.
      TestRequiredForeign message =
        TestRequiredForeign.CreateBuilder()
          .SetOptionalMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .BuildPartial();
      Assert.IsFalse(message.IsInitialized);
    }

    [Test]
    public void ParseUnititialized() {
      try {
        TestRequired.ParseFrom(ByteString.Empty);
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual("Message missing required fields: a, b, c", e.Message);
      }
    }

    [Test]
    public void ParseNestedUnititialized() {
      ByteString data =
        TestRequiredForeign.CreateBuilder()
          .SetOptionalMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .AddRepeatedMessage(TestRequiredUninitialized)
          .BuildPartial().ToByteString();

      try {
        TestRequiredForeign.ParseFrom(data);
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual(
          "Message missing required fields: " +
          "optional_message.a, " +
          "optional_message.b, " +
          "optional_message.c, " +
          "repeated_message[0].a, " +
          "repeated_message[0].b, " +
          "repeated_message[0].c, " +
          "repeated_message[1].a, " +
          "repeated_message[1].b, " +
          "repeated_message[1].c",
          e.Message);
      }
    }

    [Test]
    public void DynamicUninitializedException() {
      try {
        DynamicMessage.CreateBuilder(TestRequired.Descriptor).Build();
        Assert.Fail("Should have thrown an exception.");
      } catch (UninitializedMessageException e) {
        Assert.AreEqual("Message missing required fields: a, b, c", e.Message);
      }
    }

    [Test]
    public void DynamicBuildPartial() {
      // We're mostly testing that no exception is thrown.
      DynamicMessage message = DynamicMessage.CreateBuilder(TestRequired.Descriptor).BuildPartial();
      Assert.IsFalse(message.Initialized);
    }

    [Test]
    public void DynamicParseUnititialized() {
      try {
        MessageDescriptor descriptor = TestRequired.Descriptor;
        DynamicMessage.ParseFrom(descriptor, ByteString.Empty);
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual("Message missing required fields: a, b, c", e.Message);
      }
    }

    [Test]
    public void PackedTypesWrittenDirectlyToStream() {
      TestPackedTypes message = new TestPackedTypes.Builder {PackedInt32List = {0, 1, 2}}.Build();
      MemoryStream stream = new MemoryStream();
      message.WriteTo(stream);
      stream.Position = 0;
      TestPackedTypes readMessage = TestPackedTypes.ParseFrom(stream);
      Assert.AreEqual(message, readMessage);
    }
  }

}
