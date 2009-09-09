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
using Google.ProtocolBuffers.Descriptors;
using NUnit.Framework;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class AbstractMessageTest {

    [Test]
    public void Clear() {
      AbstractMessageWrapper message = new AbstractMessageWrapper.Builder(TestAllTypes.CreateBuilder(TestUtil.GetAllSet())).Clear().Build();
      TestUtil.AssertClear((TestAllTypes) message.WrappedMessage);
    }

    [Test]
    public void Copy() {
      AbstractMessageWrapper message = new AbstractMessageWrapper.Builder(TestAllTypes.CreateBuilder()).MergeFrom(TestUtil.GetAllSet()).Build();
      TestUtil.AssertAllFieldsSet((TestAllTypes) message.WrappedMessage);
    }

    [Test]
    public void SerializedSize() {
      TestAllTypes message = TestUtil.GetAllSet();
      IMessage abstractMessage = new AbstractMessageWrapper(TestUtil.GetAllSet());

      Assert.AreEqual(message.SerializedSize, abstractMessage.SerializedSize);
    }

    [Test]
    public void Serialization() {
      IMessage abstractMessage = new AbstractMessageWrapper(TestUtil.GetAllSet());
      TestUtil.AssertAllFieldsSet(TestAllTypes.ParseFrom(abstractMessage.ToByteString()));
      Assert.AreEqual(TestUtil.GetAllSet().ToByteString(), abstractMessage.ToByteString());
    }

    [Test]
    public void Parsing() {
      IBuilder builder = new AbstractMessageWrapper.Builder(TestAllTypes.CreateBuilder());
      AbstractMessageWrapper message = (AbstractMessageWrapper) builder.WeakMergeFrom(TestUtil.GetAllSet().ToByteString()).WeakBuild();
      TestUtil.AssertAllFieldsSet((TestAllTypes) message.WrappedMessage);
    }

    [Test]
    public void PackedSerialization() {
      IMessage abstractMessage = new AbstractMessageWrapper(TestUtil.GetPackedSet());
      TestUtil.AssertPackedFieldsSet(TestPackedTypes.ParseFrom(abstractMessage.ToByteString()));
      Assert.AreEqual(TestUtil.GetPackedSet().ToByteString(), abstractMessage.ToByteString());
    }

    [Test]
    public void PackedParsing() {
      AbstractMessageWrapper.Builder builder = new AbstractMessageWrapper.Builder(TestPackedTypes.CreateBuilder());
      AbstractMessageWrapper message = builder.MergeFrom(TestUtil.GetPackedSet().ToByteString()).Build();
      TestUtil.AssertPackedFieldsSet((TestPackedTypes)message.WrappedMessage);
    }

    [Test]
    public void OptimizedForSize() {
      // We're mostly only Checking that this class was compiled successfully.
      TestOptimizedForSize message = TestOptimizedForSize.CreateBuilder().SetI(1).Build();
      message = TestOptimizedForSize.ParseFrom(message.ToByteString());
      Assert.AreEqual(2, message.SerializedSize);
    }

    // -----------------------------------------------------------------
    // Tests for isInitialized().

    private static readonly TestRequired TestRequiredUninitialized = TestRequired.DefaultInstance;
    private static readonly TestRequired TestRequiredInitialized = TestRequired.CreateBuilder().SetA(1).SetB(2).SetC(3).Build();

    [Test]
    public void IsInitialized() {
      TestRequired.Builder builder = TestRequired.CreateBuilder();
      AbstractMessageWrapper.Builder abstractBuilder = new AbstractMessageWrapper.Builder(builder);

      Assert.IsFalse(abstractBuilder.IsInitialized);
      builder.A = 1;
      Assert.IsFalse(abstractBuilder.IsInitialized);
      builder.B = 1;
      Assert.IsFalse(abstractBuilder.IsInitialized);
      builder.C = 1;
      Assert.IsTrue(abstractBuilder.IsInitialized);
    }

    [Test]
    public void ForeignIsInitialized() {
      TestRequiredForeign.Builder builder = TestRequiredForeign.CreateBuilder();
      AbstractMessageWrapper.Builder abstractBuilder = new AbstractMessageWrapper.Builder(builder);

      Assert.IsTrue(abstractBuilder.IsInitialized);

      builder.SetOptionalMessage(TestRequiredUninitialized);
      Assert.IsFalse(abstractBuilder.IsInitialized);

      builder.SetOptionalMessage(TestRequiredInitialized);
      Assert.IsTrue(abstractBuilder.IsInitialized);

      builder.AddRepeatedMessage(TestRequiredUninitialized);
      Assert.IsFalse(abstractBuilder.IsInitialized);

      builder.SetRepeatedMessage(0, TestRequiredInitialized);
      Assert.IsTrue(abstractBuilder.IsInitialized);
    }

    // -----------------------------------------------------------------
    // Tests for mergeFrom

    static readonly TestAllTypes MergeSource = TestAllTypes.CreateBuilder()
        .SetOptionalInt32(1)
        .SetOptionalString("foo")
        .SetOptionalForeignMessage(ForeignMessage.DefaultInstance)
        .AddRepeatedString("bar")
        .Build();

    static readonly TestAllTypes MergeDest = TestAllTypes.CreateBuilder()
        .SetOptionalInt64(2)
        .SetOptionalString("baz")
        .SetOptionalForeignMessage(ForeignMessage.CreateBuilder().SetC(3).Build())
        .AddRepeatedString("qux")
        .Build();

    const string MergeResultText = "optional_int32: 1\n" +
        "optional_int64: 2\n" +
        "optional_string: \"foo\"\n" +
        "optional_foreign_message {\n" +
        "  c: 3\n" +
        "}\n" +
        "repeated_string: \"qux\"\n" +
        "repeated_string: \"bar\"\n";

    [Test]
    public void MergeFrom() {
      AbstractMessageWrapper result = (AbstractMessageWrapper) 
        new AbstractMessageWrapper.Builder(TestAllTypes.CreateBuilder(MergeDest))
            .MergeFrom(MergeSource)
            .Build();

      Assert.AreEqual(MergeResultText, result.ToString());
    }

    // -----------------------------------------------------------------
    // Tests for equals and hashCode
    
    [Test]
    public void EqualsAndHashCode() {
      TestAllTypes a = TestUtil.GetAllSet();
      TestAllTypes b = TestAllTypes.CreateBuilder().Build();
      TestAllTypes c = TestAllTypes.CreateBuilder(b).AddRepeatedString("x").Build();
      TestAllTypes d = TestAllTypes.CreateBuilder(c).AddRepeatedString("y").Build();
      TestAllExtensions e = TestUtil.GetAllExtensionsSet();
      TestAllExtensions f = TestAllExtensions.CreateBuilder(e)
          .AddExtension(UnitTestProtoFile.RepeatedInt32Extension, 999).Build();
        
      CheckEqualsIsConsistent(a);
      CheckEqualsIsConsistent(b);
      CheckEqualsIsConsistent(c);
      CheckEqualsIsConsistent(d);
      CheckEqualsIsConsistent(e);
      CheckEqualsIsConsistent(f);
      
      CheckNotEqual(a, b);
      CheckNotEqual(a, c);
      CheckNotEqual(a, d);
      CheckNotEqual(a, e);
      CheckNotEqual(a, f);

      CheckNotEqual(b, c);
      CheckNotEqual(b, d);
      CheckNotEqual(b, e);
      CheckNotEqual(b, f);

      CheckNotEqual(c, d);
      CheckNotEqual(c, e);
      CheckNotEqual(c, f);

      CheckNotEqual(d, e);
      CheckNotEqual(d, f);

      CheckNotEqual(e, f);

      // Deserializing into the TestEmptyMessage such that every field is an UnknownFieldSet.Field
      TestEmptyMessage eUnknownFields = TestEmptyMessage.ParseFrom(e.ToByteArray());
      TestEmptyMessage fUnknownFields = TestEmptyMessage.ParseFrom(f.ToByteArray());
      CheckNotEqual(eUnknownFields, fUnknownFields);
      CheckEqualsIsConsistent(eUnknownFields);
      CheckEqualsIsConsistent(fUnknownFields);

      // Subseqent reconstitutions should be identical
      TestEmptyMessage eUnknownFields2 = TestEmptyMessage.ParseFrom(e.ToByteArray());
      CheckEqualsIsConsistent(eUnknownFields, eUnknownFields2);
    }
    
    /// <summary>
    /// Asserts that the given protos are equal and have the same hash code.
    /// </summary>
    private static void CheckEqualsIsConsistent(IMessage message) {
      // Object should be equal to itself.
      Assert.AreEqual(message, message);
      
      // Object should be equal to a dynamic copy of itself.
      DynamicMessage dynamic = DynamicMessage.CreateBuilder(message).Build();
      CheckEqualsIsConsistent(message, dynamic);
    }

    /// <summary>
    /// Asserts that the given protos are equal and have the same hash code.
    /// </summary>
    private static void CheckEqualsIsConsistent(IMessage message1, IMessage message2) {
      Assert.AreEqual(message1, message2);
      Assert.AreEqual(message2, message1);
      Assert.AreEqual(message2.GetHashCode(), message1.GetHashCode());
    }

    /// <summary>
    /// Asserts that the given protos are not equal and have different hash codes.
    /// </summary>
    /// <remarks>
    /// It's valid for non-equal objects to have the same hash code, so
    /// this test is stricter than it needs to be. However, this should happen
    /// relatively rarely. (If this test fails, it's probably still due to a bug.)
    /// </remarks>
    private static void CheckNotEqual(IMessage m1, IMessage m2) {
      String equalsError = string.Format("{0} should not be equal to {1}", m1, m2);
      Assert.IsFalse(m1.Equals(m2), equalsError);
      Assert.IsFalse(m2.Equals(m1), equalsError);

      Assert.IsFalse(m1.GetHashCode() == m2.GetHashCode(),
        string.Format("{0} should have a different hash code from {1}", m1, m2));
    }

    /// <summary>
    /// Extends AbstractMessage and wraps some other message object.  The methods
    /// of the Message interface which aren't explicitly implemented by
    /// AbstractMessage are forwarded to the wrapped object.  This allows us to
    /// test that AbstractMessage's implementations work even if the wrapped
    /// object does not use them.
    /// </summary>
    private class AbstractMessageWrapper : AbstractMessage<AbstractMessageWrapper, AbstractMessageWrapper.Builder> {
      private readonly IMessage wrappedMessage;

      public IMessage WrappedMessage {
        get { return wrappedMessage; }
      }

      public AbstractMessageWrapper(IMessage wrappedMessage) {
        this.wrappedMessage = wrappedMessage;
      }

      public override MessageDescriptor DescriptorForType {
        get { return wrappedMessage.DescriptorForType; }
      }

      public override AbstractMessageWrapper DefaultInstanceForType {
        get { return new AbstractMessageWrapper(wrappedMessage.WeakDefaultInstanceForType); }
      }

      public override IDictionary<FieldDescriptor, object> AllFields {
        get { return wrappedMessage.AllFields; }
      }

      public override bool HasField(FieldDescriptor field) {
        return wrappedMessage.HasField(field);
      }
    
      public override object this[FieldDescriptor field] {
        get { return wrappedMessage[field]; }
      }

      public override object this[FieldDescriptor field, int index] {
        get { return wrappedMessage[field, index]; }
      }

      public override int GetRepeatedFieldCount(FieldDescriptor field) {
        return wrappedMessage.GetRepeatedFieldCount(field);
      }
      
      public override UnknownFieldSet UnknownFields {
        get { return wrappedMessage.UnknownFields; }
      }

      public override Builder CreateBuilderForType() {
        return new Builder(wrappedMessage.WeakCreateBuilderForType());
      }

      public override Builder ToBuilder() {
        return new Builder(wrappedMessage.WeakToBuilder());
      }
      
      internal class Builder : AbstractBuilder<AbstractMessageWrapper, Builder> {
        private readonly IBuilder wrappedBuilder;

        protected override Builder ThisBuilder {
          get { return this; }
        }

        internal Builder(IBuilder wrappedBuilder) {
          this.wrappedBuilder = wrappedBuilder;
        }

        public override Builder MergeFrom(AbstractMessageWrapper other) {
          wrappedBuilder.WeakMergeFrom(other.wrappedMessage);
          return this;
        }

        public override bool IsInitialized {
          get { return wrappedBuilder.IsInitialized; }
        }

        public override IDictionary<FieldDescriptor, object> AllFields {
          get { return wrappedBuilder.AllFields; }
        }

        public override object this[FieldDescriptor field] {
          get { return wrappedBuilder[field]; }
          set { wrappedBuilder[field] = value; }
        }

        public override MessageDescriptor DescriptorForType {
          get { return wrappedBuilder.DescriptorForType; }
        }

        public override int GetRepeatedFieldCount(FieldDescriptor field) {
          return wrappedBuilder.GetRepeatedFieldCount(field);
        }

        public override object this[FieldDescriptor field, int index] {
          get { return wrappedBuilder[field, index]; }
          set { wrappedBuilder[field, index] = value; }
        }

        public override bool HasField(FieldDescriptor field) {
          return wrappedBuilder.HasField(field);
        }

        public override UnknownFieldSet UnknownFields {
          get { return wrappedBuilder.UnknownFields; }
          set { wrappedBuilder.UnknownFields = value; }
        }

        public override AbstractMessageWrapper Build() {
          return new AbstractMessageWrapper(wrappedBuilder.WeakBuild());
        }

        public override AbstractMessageWrapper BuildPartial() {
          return new AbstractMessageWrapper(wrappedBuilder.WeakBuildPartial());
        }

        public override Builder Clone() {
          return new Builder(wrappedBuilder.WeakClone());
        }

        public override AbstractMessageWrapper DefaultInstanceForType {
          get { return new AbstractMessageWrapper(wrappedBuilder.WeakDefaultInstanceForType); }
        }

        public override Builder ClearField(FieldDescriptor field) {
          wrappedBuilder.WeakClearField(field);
          return this;
        }

        public override Builder AddRepeatedField(FieldDescriptor field, object value) {
          wrappedBuilder.WeakAddRepeatedField(field, value);
          return this;
        }

        public override IBuilder CreateBuilderForField(FieldDescriptor field) {
          wrappedBuilder.CreateBuilderForField(field);
          return this;
        }

        public override Builder MergeFrom(IMessage other) {
          wrappedBuilder.WeakMergeFrom(other);
          return this;
        }

        public override Builder MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
          wrappedBuilder.WeakMergeFrom(input, extensionRegistry);
          return this;
        }
      }
    }
  }
}
