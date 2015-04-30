using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Serialization;
using Google.ProtocolBuffers.Serialization.Http;
using Google.ProtocolBuffers.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class TestMimeMessageFormats
    {
        // There is a whole host of various json mime types in use around the net, this is the set we accept...
        readonly IEnumerable<string> JsonTypes = new string[] { "application/json", "application/x-json", "application/x-javascript", "text/javascript", "text/x-javascript", "text/x-json", "text/json" };
        readonly IEnumerable<string> XmlTypes = new string[] { "text/xml", "application/xml" };
        readonly IEnumerable<string> ProtobufTypes = new string[] { "application/binary", "application/x-protobuf", "application/vnd.google.protobuf" };

        [Fact]
        public void TestReadJsonMimeTypes()
        {
            foreach (string type in JsonTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is JsonFormatReader);
            }
            Assert.True(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/json" }, null, Stream.Null)
                is JsonFormatReader);
        }

        [Fact]
        public void TestWriteJsonMimeTypes()
        {
            foreach (string type in JsonTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is JsonFormatWriter);
            }
            Assert.True(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/json" }, null, Stream.Null)
                is JsonFormatWriter);
        }

        [Fact]
        public void TestReadXmlMimeTypes()
        {
            foreach (string type in XmlTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is XmlFormatReader);
            }
            Assert.True(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/xml" }, null, Stream.Null)
                is XmlFormatReader);
        }

        [Fact]
        public void TestWriteXmlMimeTypes()
        {
            foreach (string type in XmlTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is XmlFormatWriter);
            }
            Assert.True(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/xml" }, null, Stream.Null)
                is XmlFormatWriter);
        }

        [Fact]
        public void TestReadProtoMimeTypes()
        {
            foreach (string type in ProtobufTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateInputStream(new MessageFormatOptions(), type, Stream.Null)
                    is CodedInputStream);
            }
            Assert.True(
                MessageFormatFactory.CreateInputStream(new MessageFormatOptions() { DefaultContentType = "application/vnd.google.protobuf" }, null, Stream.Null)
                is CodedInputStream);
        }

        [Fact]
        public void TestWriteProtoMimeTypes()
        {
            foreach (string type in ProtobufTypes)
            {
                Assert.True(
                    MessageFormatFactory.CreateOutputStream(new MessageFormatOptions(), type, Stream.Null)
                    is CodedOutputStream);
            }
            Assert.True(
                MessageFormatFactory.CreateOutputStream(new MessageFormatOptions() { DefaultContentType = "application/vnd.google.protobuf" }, null, Stream.Null)
                is CodedOutputStream);
        }

        [Fact]
        public void TestMergeFromJsonType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/json", new MemoryStream(Encoding.UTF8.GetBytes(
                    Extensions.ToJson(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                    )))
                .Build();
            Assert.Equal("a", msg.Text);
            Assert.Equal(1, msg.Number);
        }

        [Fact]
        public void TestMergeFromXmlType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/xml", new MemoryStream(Encoding.UTF8.GetBytes(
                    Extensions.ToXml(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                    )))
                .Build();
            Assert.Equal("a", msg.Text);
            Assert.Equal(1, msg.Number);
        }
        [Fact]
        public void TestMergeFromProtoType()
        {
            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                new MessageFormatOptions(), "application/vnd.google.protobuf", new MemoryStream(
                    TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build().ToByteArray()
                    ))
                .Build();
            Assert.Equal("a", msg.Text);
            Assert.Equal(1, msg.Number);
        }

        [Fact]
        public void TestWriteToJsonType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/json", ms);

            Assert.Equal(@"{""text"":""a"",""number"":1}", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Fact]
        public void TestWriteToXmlType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/xml", ms);

            Assert.Equal("<root><text>a</text><number>1</number></root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }
        [Fact]
        public void TestWriteToProtoType()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions(), "application/vnd.google.protobuf", ms);

            byte[] bytes = TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build().ToByteArray();
            Assert.Equal(bytes, ms.ToArray());
        }

        [Fact]
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

            Assert.Equal("a", msg.Text);
            Assert.Equal(1, msg.NumbersList[0]);
            Assert.Equal(2, msg.NumbersList[1]);

        }

        [Fact]
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

            Assert.Equal("a", builder.Text);
            Assert.Equal(1, builder.NumbersList[0]);
            Assert.Equal(2, builder.NumbersList[1]);
        }

        [Fact]
        public void TestJsonFormatted()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions() { FormattedOutput = true }, "application/json", ms);

            Assert.Equal("{\r\n    \"text\": \"a\",\r\n    \"number\": 1\r\n}", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Fact]
        public void TestXmlFormatted()
        {
            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                new MessageFormatOptions() { FormattedOutput = true }, "application/xml", ms);

            Assert.Equal("<root>\r\n    <text>a</text>\r\n    <number>1</number>\r\n</root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }

        [Fact]
        public void TestReadCustomMimeTypes()
        {
            var options = new MessageFormatOptions();
            //Remove existing mime-type mappings
            options.MimeInputTypes.Clear();
            //Add our own
            options.MimeInputTypes.Add("-custom-XML-mime-type-", XmlFormatReader.CreateInstance);
            Assert.Equal(1, options.MimeInputTypes.Count);

            Stream xmlStream = new MemoryStream(Encoding.UTF8.GetBytes(
                Extensions.ToXml(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build())
                ));

            TestXmlMessage msg = Extensions.MergeFrom(new TestXmlMessage.Builder(),
                options, "-custom-XML-mime-type-", xmlStream)
                .Build();
            Assert.Equal("a", msg.Text);
            Assert.Equal(1, msg.Number);
        }

        [Fact]
        public void TestWriteToCustomType()
        {
            var options = new MessageFormatOptions();
            //Remove existing mime-type mappings
            options.MimeOutputTypes.Clear();
            //Add our own
            options.MimeOutputTypes.Add("-custom-XML-mime-type-", XmlFormatWriter.CreateInstance);
            
            Assert.Equal(1, options.MimeOutputTypes.Count);

            MemoryStream ms = new MemoryStream();
            Extensions.WriteTo(TestXmlMessage.CreateBuilder().SetText("a").SetNumber(1).Build(),
                options, "-custom-XML-mime-type-", ms);

            Assert.Equal("<root><text>a</text><number>1</number></root>", Encoding.UTF8.GetString(ms.ToArray(), 0, (int)ms.Length));
        }
    }
}