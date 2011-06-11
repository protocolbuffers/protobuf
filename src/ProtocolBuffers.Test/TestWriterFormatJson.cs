using System;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers
{
    [TestFixture]
    public class TestWriterFormatJson
    {
        protected string Content;
        [System.Diagnostics.DebuggerNonUserCode]
        protected void FormatterAssert<TMessage>(TMessage message, params string[] expecting) where TMessage : IMessageLite
        {
            StringWriter sw = new StringWriter();
            JsonFormatWriter.CreateInstance(sw).WriteMessage(message);
            
            Content = sw.ToString();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnitTestXmlSerializerTestProtoFile.RegisterAllExtensions(registry);

            IMessageLite copy = 
                JsonFormatReader.CreateInstance(Content)
                .Merge(message.WeakCreateBuilderForType(), registry).WeakBuild();

            Assert.AreEqual(typeof(TMessage), copy.GetType());
            Assert.AreEqual(message, copy);
            foreach (string expect in expecting)
                Assert.IsTrue(Content.IndexOf(expect) >= 0, "Expected to find content '{0}' in: \r\n{1}", expect, Content);
        }

        [Test]
        public void TestToJsonParseFromJson()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string json = msg.ToJson();
            Assert.AreEqual("{\"default_bool\":true}", json);
            TestAllTypes copy = TestAllTypes.ParseFromJson(json);
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [Test]
        public void TestToJsonParseFromJsonReader()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string json = msg.ToJson();
            Assert.AreEqual("{\"default_bool\":true}", json);
            TestAllTypes copy = TestAllTypes.ParseFromJson(new StringReader(json));
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [Test]
        public void TestJsonFormatted()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetNumber(0x1010)
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder())
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.ONE))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.ONE).AddOptions(EnumOptions.TWO))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().SetBinary(ByteString.CopyFromUtf8("abc")))
                .Build();

            StringWriter sw = new StringWriter();
            JsonFormatWriter.CreateInstance(sw).Formatted()
                .WriteMessage(message);

            string json = sw.ToString();

            TestXmlMessage copy = JsonFormatReader.CreateInstance(json)
                .Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }

        [Test]
        public void TestEmptyMessage()
        {
            FormatterAssert(
                TestXmlChild.CreateBuilder()
                .Build(),
                @"{}"
                );
        }
        [Test]
        public void TestRepeatedField()
        {
            FormatterAssert(
                TestXmlChild.CreateBuilder()
                .AddOptions(EnumOptions.ONE)
                .AddOptions(EnumOptions.TWO)
                .Build(),
                @"{""options"":[""ONE"",""TWO""]}"
                );
        }
        [Test]
        public void TestNestedEmptyMessage()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().Build())
                .Build(),
                @"{""child"":{}}"
                );
        }
        [Test]
        public void TestNestedMessage()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.TWO).Build())
                .Build(),
                @"{""child"":{""options"":[""TWO""]}}"
                );
        }
        [Test]
        public void TestBooleanTypes()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .Build(),
                @"{""valid"":true}"
                );
        }
        [Test]
        public void TestFullMessage()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .AddTextlines("a")
                .AddTextlines("b")
                .AddTextlines("c")
                .SetNumber(0x1010101010)
                .AddNumbers(1)
                .AddNumbers(2)
                .AddNumbers(3)
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.ONE).SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.TWO).SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.THREE).SetBinary(ByteString.CopyFrom(new byte[3])))
                .Build(),
                @"""text"":""text""",
                @"[""a"",""b"",""c""]",
                @"[1,2,3]",
                @"""child"":{",
                @"""children"":[{",
                @"AA==",
                @"AAA=",
                @"AAAA",
                0x1010101010L.ToString()
                );
        }
        [Test]
        public void TestMessageWithXmlText()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetText("<text></text>")
                .Build(),
                @"{""text"":""<text><\/text>""}"
                );
        }
        [Test]
        public void TestWithEscapeChars()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetText(" \t <- \"leading space and trailing\" -> \\ \xef54 \x0000 \xFF \xFFFF \b \f \r \n \t ")
                .Build(),
                "{\"text\":\" \\t <- \\\"leading space and trailing\\\" -> \\\\ \\uef54 \\u0000 \\u00ff \\uffff \\b \\f \\r \\n \\t \"}"
                );
        }
        [Test]
        public void TestWithExtensionText()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetValid(false)
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionText, " extension text value ! ")
                .Build(),
                @"{""valid"":false,""extension_text"":"" extension text value ! ""}"
                );
        }
        [Test]
        public void TestWithExtensionNumber()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionMessage,
                new TestXmlExtension.Builder().SetNumber(42).Build())
                .Build(),
                @"{""number"":42}"
                );
        }
        [Test]
        public void TestWithExtensionArray()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 100)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 101)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 102)
                .Build(),
                @"{""extension_number"":[100,101,102]}"
                );
        }
        [Test]
        public void TestWithExtensionEnum()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum, EnumOptions.ONE)
                .Build(),
                @"{""extension_enum"":""ONE""}"
                );
        }
        [Test]
        public void TestMessageWithExtensions()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionText, "extension text")
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionMessage, new TestXmlExtension.Builder().SetNumber(42).Build())
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 100)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 101)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 102)
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum, EnumOptions.ONE)
                .Build(),
                @"""text"":""text""",
                @"""valid"":true",
                @"""extension_enum"":""ONE""",
                @"""extension_text"":""extension text""",
                @"""extension_number"":[100,101,102]",
                @"""extension_message"":{""number"":42}"
                );
        }
        [Test]
        public void TestMessageMissingExtensions()
        {
            TestXmlMessage original = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionText, " extension text value ! ")
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionMessage, new TestXmlExtension.Builder().SetNumber(42).Build())
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 100)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 101)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 102)
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum, EnumOptions.ONE)
                .Build();

            TestXmlMessage message = original.ToBuilder()
                .ClearExtension(UnitTestXmlSerializerTestProtoFile.ExtensionText)
                .ClearExtension(UnitTestXmlSerializerTestProtoFile.ExtensionMessage)
                .ClearExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber)
                .ClearExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum)
                .Build();

            JsonFormatWriter writer = JsonFormatWriter.CreateInstance();
            writer.WriteMessage(original);
            Content = writer.ToString();

            IMessageLite copy = JsonFormatReader.CreateInstance(Content)
                .Merge(message.CreateBuilderForType()).Build();

            Assert.AreNotEqual(original, message);
            Assert.AreNotEqual(original, copy);
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestMergeFields()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(JsonFormatReader.CreateInstance("\"valid\": true"));
            builder.MergeFrom(JsonFormatReader.CreateInstance("\"text\": \"text\", \"number\": \"411\""));
            Assert.AreEqual(true, builder.Valid);
            Assert.AreEqual("text", builder.Text);
            Assert.AreEqual(411, builder.Number);
        }
        [Test]
        public void TestMessageArray()
        {
            JsonFormatWriter writer = JsonFormatWriter.CreateInstance().Formatted();
            using (writer.StartArray())
            {
                writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(1).AddTextlines("a").Build());
                writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(2).AddTextlines("b").Build());
                writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(3).AddTextlines("c").Build());
            }
            string json = writer.ToString();
            JsonFormatReader reader = JsonFormatReader.CreateInstance(json);
            
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            int ordinal = 0;

            foreach (JsonFormatReader r in reader.EnumerateArray())
            {
                r.Merge(builder);
                Assert.AreEqual(++ordinal, builder.Number);
            }
            Assert.AreEqual(3, ordinal);
            Assert.AreEqual(3, builder.TextlinesCount);
        }
        [Test]
        public void TestNestedMessageArray()
        {
            JsonFormatWriter writer = JsonFormatWriter.CreateInstance();
            using (writer.StartArray())
            {
                using (writer.StartArray())
                {
                    writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(1).AddTextlines("a").Build());
                    writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(2).AddTextlines("b").Build());
                } 
                using (writer.StartArray())
                    writer.WriteMessage(TestXmlMessage.CreateBuilder().SetNumber(3).AddTextlines("c").Build());
            }
            string json = writer.ToString();
            JsonFormatReader reader = JsonFormatReader.CreateInstance(json);

            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            int ordinal = 0;

            foreach (JsonFormatReader r in reader.EnumerateArray())
                foreach (JsonFormatReader r2 in r.EnumerateArray())
                {
                    r2.Merge(builder);
                    Assert.AreEqual(++ordinal, builder.Number);
                }
            Assert.AreEqual(3, ordinal);
            Assert.AreEqual(3, builder.TextlinesCount);
        }
        [Test,ExpectedException(typeof(InvalidProtocolBufferException))]
        public void TestRecursiveLimit()
        {
            StringBuilder sb = new StringBuilder(8192);
            for (int i = 0; i < 80; i++)
                sb.Append("{\"child\":");
            TestXmlRescursive msg = TestXmlRescursive.ParseFromJson(sb.ToString());
        }
        [Test, ExpectedException(typeof(FormatException))]
        public void FailWithEmptyText()
        {
            JsonFormatReader.CreateInstance("")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        [Test, ExpectedException(typeof(FormatException))]
        public void FailWithUnexpectedValue()
        {
            JsonFormatReader.CreateInstance("{{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        [Test, ExpectedException(typeof(FormatException))]
        public void FailWithUnQuotedName()
        {
            JsonFormatReader.CreateInstance("{name:{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        [Test, ExpectedException(typeof(FormatException))]
        public void FailWithUnexpectedType()
        {
            JsonFormatReader.CreateInstance("{\"valid\":{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }
    }
}
