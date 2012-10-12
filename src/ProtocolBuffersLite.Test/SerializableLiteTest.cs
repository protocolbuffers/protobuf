using System;
#if !SILVERLIGHT && !COMPACT_FRAMEWORK
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    [TestClass]
    public class SerializableLiteTest
    {
        /// <summary>
        /// Just keep it from even compiling if we these objects don't implement the expected interface.
        /// </summary>
        public static readonly ISerializable CompileTimeCheckSerializableMessage = TestRequiredLite.DefaultInstance;
        public static readonly ISerializable CompileTimeCheckSerializableBuilder = new TestRequiredLite.Builder();

        [TestMethod]
        public void TestPlainMessage()
        {
            TestRequiredLite message = TestRequiredLite.CreateBuilder()
                .SetD(42)
                .BuildPartial();

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, message);

            ms.Position = 0;
            TestRequiredLite copy = (TestRequiredLite)new BinaryFormatter().Deserialize(ms);

            Assert.AreEqual(message, copy);
        }

        [TestMethod]
        public void TestPlainBuilder()
        {
            TestRequiredLite.Builder builder = TestRequiredLite.CreateBuilder()
                .SetD(42)
                ;

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, builder);

            ms.Position = 0;
            TestRequiredLite.Builder copy = (TestRequiredLite.Builder)new BinaryFormatter().Deserialize(ms);

            Assert.AreEqual(builder.BuildPartial(), copy.BuildPartial());
        }
    }
}
#endif