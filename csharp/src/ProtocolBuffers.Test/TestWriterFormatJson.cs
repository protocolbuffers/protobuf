using System;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Serialization;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Google.ProtocolBuffers.TestProtos;
using EnumOptions = Google.ProtocolBuffers.TestProtos.EnumOptions;

namespace Google.ProtocolBuffers
{
    [TestClass]
    public class TestWriterFormatJson
    {
        [TestMethod]
        public void Example_FromJson()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();

            //3.5: builder.MergeFromJson(@"{""valid"":true}");
            Extensions.MergeFromJson(builder, @"{""valid"":true}");
            
            TestXmlMessage message = builder.Build();
            Assert.AreEqual(true, message.Valid);
        }

        [TestMethod]
        public void Example_ToJson()
        {
            TestXmlMessage message = 
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .Build();

            //3.5: string json = message.ToJson();
            string json = Extensions.ToJson(message);

            Assert.AreEqual(@"{""valid"":true}", json);
        }

        [TestMethod]
        public void Example_WriteJsonUsingICodedOutputStream()
        {
            TestXmlMessage message =
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .Build();

            using (TextWriter output = new StringWriter())
            {
                ICodedOutputStream writer = JsonFormatWriter.CreateInstance(output);
                writer.WriteMessageStart();      //manually begin the message, output is '{'
                
                writer.Flush();
                Assert.AreEqual("{", output.ToString());

                ICodedOutputStream stream = writer;
                message.WriteTo(stream);    //write the message normally

                writer.Flush();
                Assert.AreEqual(@"{""valid"":true", output.ToString());

                writer.WriteMessageEnd();        //manually write the end message '}'
                Assert.AreEqual(@"{""valid"":true}", output.ToString());
            }
        }

        [TestMethod]
        public void Example_ReadJsonUsingICodedInputStream()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            ICodedInputStream reader = JsonFormatReader.CreateInstance(@"{""valid"":true}");

            reader.ReadMessageStart();  //manually read the begin the message '{'

            builder.MergeFrom(reader);  //write the message normally

            reader.ReadMessageEnd();    //manually read the end message '}'
        }

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

        [TestMethod]
        public void TestToJsonParseFromJson()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string json = Extensions.ToJson(msg);
            Assert.AreEqual("{\"default_bool\":true}", json);
            TestAllTypes copy = Extensions.MergeFromJson(new TestAllTypes.Builder(), json).Build();
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [TestMethod]
        public void TestToJsonParseFromJsonReader()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string json = Extensions.ToJson(msg);
            Assert.AreEqual("{\"default_bool\":true}", json);
            TestAllTypes copy = Extensions.MergeFromJson(new TestAllTypes.Builder(), new StringReader(json)).Build();
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [TestMethod]
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

        [TestMethod]
        public void TestEmptyMessage()
        {
            FormatterAssert(
                TestXmlChild.CreateBuilder()
                .Build(),
                @"{}"
                );
        }
        
        [TestMethod]
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
        
        [TestMethod]
        public void TestNestedEmptyMessage()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().Build())
                .Build(),
                @"{""child"":{}}"
                );
        }
        
        [TestMethod]
        public void TestNestedMessage()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.TWO).Build())
                .Build(),
                @"{""child"":{""options"":[""TWO""]}}"
                );
        }
        
        [TestMethod]
        public void TestBooleanTypes()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .Build(),
                @"{""valid"":true}"
                );
        }
        
        [TestMethod]
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
        
        [TestMethod]
        public void TestMessageWithXmlText()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetText("<text></text>")
                .Build(),
                @"{""text"":""<text><\/text>""}"
                );
        }
        
        [TestMethod]
        public void TestWithEscapeChars()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetText(" \t <- \"leading space and trailing\" -> \\ \xef54 \x0000 \xFF \xFFFF \b \f \r \n \t ")
                .Build(),
                "{\"text\":\" \\t <- \\\"leading space and trailing\\\" -> \\\\ \\uef54 \\u0000 \\u00ff \\uffff \\b \\f \\r \\n \\t \"}"
                );
        }
        
        [TestMethod]
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
        
        [TestMethod]
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
        
        [TestMethod]
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
        
        [TestMethod]
        public void TestWithExtensionEnum()
        {
            FormatterAssert(
                TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum, EnumOptions.ONE)
                .Build(),
                @"{""extension_enum"":""ONE""}"
                );
        }
        
        [TestMethod]
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
        
        [TestMethod]
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
        
        [TestMethod]
        public void TestMergeFields()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(JsonFormatReader.CreateInstance("\"valid\": true"));
            builder.MergeFrom(JsonFormatReader.CreateInstance("\"text\": \"text\", \"number\": \"411\""));
            Assert.AreEqual(true, builder.Valid);
            Assert.AreEqual("text", builder.Text);
            Assert.AreEqual(411, builder.Number);
        }
        
        [TestMethod]
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
        
        [TestMethod]
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
        
        [TestMethod]
        public void TestReadWriteJsonWithoutRoot()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            TestXmlMessage message = builder.SetText("abc").SetNumber(123).Build();

            string Json;
            using (StringWriter sw = new StringWriter())
            {
                ICodedOutputStream output = JsonFormatWriter.CreateInstance(sw);

                message.WriteTo(output);
                output.Flush();
                Json = sw.ToString();
            }
            Assert.AreEqual(@"""text"":""abc"",""number"":123", Json);

            ICodedInputStream input = JsonFormatReader.CreateInstance(Json);
            TestXmlMessage copy = TestXmlMessage.CreateBuilder().MergeFrom(input).Build();

            Assert.AreEqual(message, copy);
        }
        
        [TestMethod,ExpectedException(typeof(RecursionLimitExceededException))]
        public void TestRecursiveLimit()
        {
            StringBuilder sb = new StringBuilder(8192);
            for (int i = 0; i < 80; i++)
                sb.Append("{\"child\":");
            TestXmlRescursive msg = Extensions.MergeFromJson(new TestXmlRescursive.Builder(), sb.ToString()).Build();
        }

        [TestMethod, ExpectedException(typeof(FormatException))]
        public void FailWithEmptyText()
        {
            JsonFormatReader.CreateInstance("")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        
        [TestMethod, ExpectedException(typeof(FormatException))]
        public void FailWithUnexpectedValue()
        {
            JsonFormatReader.CreateInstance("{{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        
        [TestMethod, ExpectedException(typeof(FormatException))]
        public void FailWithUnQuotedName()
        {
            JsonFormatReader.CreateInstance("{name:{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }
        
        [TestMethod, ExpectedException(typeof(FormatException))]
        public void FailWithUnexpectedType()
        {
            JsonFormatReader.CreateInstance("{\"valid\":{}}")
                .Merge(TestXmlMessage.CreateBuilder());
        }

        // See issue 64 for background.
        [TestMethod]
        public void ToJsonRequiringBufferExpansion()
        {
            string s = new string('.', 4086);
            var opts = FileDescriptorProto.CreateBuilder()
               .SetName(s)
               .SetPackage("package")
               .BuildPartial();

            Assert.IsNotNull(Extensions.ToJson(opts));
        }
    }
}
