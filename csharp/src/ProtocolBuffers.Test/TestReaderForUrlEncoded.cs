using System;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using Google.ProtocolBuffers.Serialization.Http;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    public class TestReaderForUrlEncoded
    {
        [Test]
        public void Example_FromQueryString()
        {
            Uri sampleUri = new Uri("http://sample.com/Path/File.ext?text=two+three%20four&valid=true&numbers=1&numbers=2", UriKind.Absolute);

            ICodedInputStream input = FormUrlEncodedReader.CreateInstance(sampleUri.Query);

            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);
            
            TestXmlMessage message = builder.Build();
            Assert.AreEqual(true, message.Valid);
            Assert.AreEqual("two three four", message.Text);
            Assert.AreEqual(2, message.NumbersCount);
            Assert.AreEqual(1, message.NumbersList[0]);
            Assert.AreEqual(2, message.NumbersList[1]);
        }

        [Test]
        public void Example_FromFormData()
        {
            Stream rawPost = new MemoryStream(Encoding.UTF8.GetBytes("text=two+three%20four&valid=true&numbers=1&numbers=2"), false);

            ICodedInputStream input = FormUrlEncodedReader.CreateInstance(rawPost);

            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            TestXmlMessage message = builder.Build();
            Assert.AreEqual(true, message.Valid);
            Assert.AreEqual("two three four", message.Text);
            Assert.AreEqual(2, message.NumbersCount);
            Assert.AreEqual(1, message.NumbersList[0]);
            Assert.AreEqual(2, message.NumbersList[1]);
        }

        [Test]
        public void TestEmptyValues()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("valid=true&text=&numbers=1");
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            Assert.IsTrue(builder.Valid);
            Assert.IsTrue(builder.HasText);
            Assert.AreEqual("", builder.Text);
            Assert.AreEqual(1, builder.NumbersCount);
            Assert.AreEqual(1, builder.NumbersList[0]);
        }

        [Test]
        public void TestNoValue()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("valid=true&text&numbers=1");
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder();
            builder.MergeFrom(input);

            Assert.IsTrue(builder.Valid);
            Assert.IsTrue(builder.HasText);
            Assert.AreEqual("", builder.Text);
            Assert.AreEqual(1, builder.NumbersCount);
            Assert.AreEqual(1, builder.NumbersList[0]);
        }

        [Test]
        public void FormUrlEncodedReaderDoesNotSupportChildren()
        {
            ICodedInputStream input = FormUrlEncodedReader.CreateInstance("child=uh0");
            Assert.Throws<NotSupportedException>(() => TestXmlMessage.CreateBuilder().MergeFrom(input));
        }
    }
}
