using System;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using Google.ProtocolBuffers.Serialization.Http;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class TestReaderForUrlEncoded
    {
        [Fact]
        public void Example_FromQueryString()
        {
            Uri sampleUri = new Uri("http://sample.com/Path/File.ext?text=two+three%20four&valid=true&numbers=1&numbers=2", UriKind.Absolute);

            ICodedInputStream input = FormUrlEncodedReader.CreateInstance(sampleUri.Query);

            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);
            
            TestXmlMessage message = builder.Build();
            Assert.Equal(true, message.Valid);
            Assert.Equal("two three four", message.Text);
            Assert.Equal(2, message.NumbersCount);
            Assert.Equal(1, message.NumbersList[0]);
            Assert.Equal(2, message.NumbersList[1]);
        }

        [Fact]
        public void Example_FromFormData()
        {
            Stream rawPost = new MemoryStream(Encoding.UTF8.GetBytes("text=two+three%20four&valid=true&numbers=1&numbers=2"), false);

            ICodedInputStream input = FormUrlEncodedReader.CreateInstance(rawPost);

            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            TestXmlMessage message = builder.Build();
            Assert.Equal(true, message.Valid);
            Assert.Equal("two three four", message.Text);
            Assert.Equal(2, message.NumbersCount);
            Assert.Equal(1, message.NumbersList[0]);
            Assert.Equal(2, message.NumbersList[1]);
        }

        [Fact]
        public void TestEmptyValues()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("valid=true&text=&numbers=1");
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            Assert.True(builder.Valid);
            Assert.True(builder.HasText);
            Assert.Equal("", builder.Text);
            Assert.Equal(1, builder.NumbersCount);
            Assert.Equal(1, builder.NumbersList[0]);
        }

        [Fact]
        public void TestNoValue()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("valid=true&text&numbers=1");
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            Assert.True(builder.Valid);
            Assert.True(builder.HasText);
            Assert.Equal("", builder.Text);
            Assert.Equal(1, builder.NumbersCount);
            Assert.Equal(1, builder.NumbersList[0]);
        }

        [Fact]
        public void FormUrlEncodedReaderDoesNotSupportChildren()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("child=uh0");
            Assert.Throws<NotSupportedException>(() => TestXmlMessage.CreateBuilder().MergeFrom(input));
        }
    }
}
