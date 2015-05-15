using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Serialization;
using Google.ProtocolBuffers.Serialization.Http;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    public class TestMimeMessageFormats
    {
        // There is a whole host of various json mime types in use around the net, this is the set we accept...
        readonly IEnumerable<string> JsonTypes = new string[] { "application/json", "application/x-json", "application/x-javascript", "text/javascript", "text/x-javascript", "text/x-json", "text/json" };
        readonly IEnumerable<string> XmlTypes = new string[] { "text/xml", "application/xml" };
        readonly IEnumerable<string> ProtobufTypes = new string[] { "application/binary", "application/x-protobuf", "application/vnd.google.protobuf" };

        [Test]
        public void TestReadJsonMimeTypes()
        {
            foreach (string type in JsonTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is JsonFormatReader);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/json" }, null, Stream.Null)
                is JsonFormatReader);
        }

        [Test]
        public void TestWriteJsonMimeTypes()
        {
            foreach (string type in JsonTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is JsonFormatWriter);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/json" }, null, Stream.Null)
                is JsonFormatWriter);
        }

        [Test]
        public void TestReadXmlMimeTypes()
        {
            foreach (string type in XmlTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is XmlFormatReader);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/xml" }, null, Stream.Null)
                is XmlFormatReader);
        }

        [Test]
        public void TestWriteXmlMimeTypes()
        {
            foreach (string type in XmlTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is XmlFormatWriter);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/xml" }, null, Stream.Null)
                is XmlFormatWriter);
        }

        [Test]
        public void TestReadProtoMimeTypes()
        {
            foreach (string type in ProtobufTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is CodedInputStream);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/vnd.google.protobuf" }, null, Stream.Null)
                is CodedInputStream);
        }

        [Test]
        public void TestWriteProtoMimeTypes()
        {
            foreach (string type in ProtobufTypes)
            {
                Assert.IsTrue(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is CodedOutputStream);
            }
            Assert.IsTrue(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/vnd.google.protobuf" }, null, Stream.Null)
                is CodedOutputStream);
        }

        [Test]
        public void TestMergeFromJsonType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/json", new MemoryStream(Encoding.UTF8.GetBytes(
                    Extensions.ToJson(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                    )))
                .Build();
            Assert.AreEqual("a", msg.Text);
            Assert.AreEqual(1, msg.Number);
        }

        [Test]
        public void TestMergeFromXmlType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/xml", new MemoryStream(Encoding.UTF8.GetBytes(
                    Extensions.ToXml(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                    )))
                .Build();
            Assert.AreEqual("a", msg.Text);
            Assert.AreEqual(1, msg.Number);
        }
        [Test]
        public void TestMergeFromProtoType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/vnd.google.protobuf", new MemoryStream(
                    TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build().ToByteArray()
                    ))
                .Build();
            Assert.AreEqual("a", msg.Text);
            Assert.AreEqual(1, msg.Number);
        }

        [Test]
        public void TestWriteToJsonType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/json", ms);

            Assert.AreEqual(@"{""text"":""a"",""number"":1}", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Test]
        public void TestWriteToXmlType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/xml", ms);

            Assert.AreEqual("<root><text>a</text><number>1</number></root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }
        [Test]
        public void TestWriteToProtoType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/vnd.google.protobuf", ms);

            byte[] bytes = TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build().ToByteArray();
            Assert.AreEqual(bytes, ms.ToArray());
        }

        [Test]
        public void TestXmlReaderOptions()
        {
            MemoryStream ms = new MemoryStream();
            XmlFormatWriter.CreateInstance(ms)
                .SetOptions(XmlWriterOptions.OutputNestedArrays)
                .WriteMessage("my-root-node", TestXmlMessage.CreateBuilder().SetText("a").AddNumbers(1).AddNumbers(2).Build());
            ms.Position = 0;

            MessageFormatOptions options = new MessageFormatOptions()
            {
                XmlReaderOptions = XmlReaderOptions.ReadNestedArrays,
                XmlReaderRootElementName = "my-root-node"
            };

            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                options, "application/xml", ms)
                .Build();

            Assert.AreEqual("a", msg.Text);
            Assert.AreEqual(1, msg.NumbersList[0]);
            Assert.AreEqual(2, msg.NumbersList[1]);

        }

        [Test]
        public void TestXmlWriterOptions()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder().SetText("a").AddNumbers(1).AddNumbers(2).Build();
            MessageFormatOptions options = new MessageFormatOptions()
            {
                XmlWriterOptions = XmlWriterOptions.OutputNestedArrays,
                XmlWriterRootElementName = "root-node"
            };

            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(message, options, "application/xml", ms);
            ms.Position = 0;
            
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            XmlFormatReader.CreateInstance(ms)
                .SetOptions(XmlReaderOptions.ReadNestedArrays)
                .Merge("root-node", builder);

            Assert.AreEqual("a", builder.Text);
            Assert.AreEqual(1, builder.NumbersList[0]);
            Assert.AreEqual(2, builder.NumbersList[1]);
        }

        [Test]
        public void TestJsonFormatted()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions() { FormattedOutput = true }, "application/json", ms);

            string expected = string.Format("{{{0}    \"text\": \"a\",{0}    \"number\": 1{0}}}", System.Environment.NewLine);
            Assert.AreEqual(expected, Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Test]
        public void TestXmlFormatted()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions() { FormattedOutput = true }, "application/xml", ms);

            Assert.AreEqual("<root>\r\n    <text>a</text>\r\n    <number>1</number>\r\n</root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Test]
        public void TestReadCustomMimeTypes()
        {
            var options = new MessageFormatOptions();
            //Remove existing mime-type mappings
            options.MimeInputTypes.Clear();
            //Add our own
            options.MimeInputTypes.Add("-custom-XML-mime-type-", XmlFormatReader.CreateInstance);
            Assert.AreEqual(1, options.MimeInputTypes.Count);

            Stream xmlStream = new MemoryStream(Encoding.UTF8.GetBytes(
                Extensions.ToXml(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                ));

            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                options, "-custom-XML-mime-type-", xmlStream)
                .Build();
            Assert.AreEqual("a", msg.Text);
            Assert.AreEqual(1, msg.Number);
        }

        [Test]
        public void TestWriteToCustomType()
        {
            var options = new MessageFormatOptions();
            //Remove existing mime-type mappings
            options.MimeOutputTypes.Clear();
            //Add our own
            options.MimeOutputTypes.Add("-custom-XML-mime-type-", XmlFormatWriter.CreateInstance);
            
            Assert.AreEqual(1, options.MimeOutputTypes.Count);

            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                options, "-custom-XML-mime-type-", ms);

            Assert.AreEqual("<root><text>a</text><number>1</number></root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }
    }
}