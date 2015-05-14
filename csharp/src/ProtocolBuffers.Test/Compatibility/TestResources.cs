using System.IO;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Compatibility
{
    static class TestResources
    {
        public static byte[] google_message1
        { 
            get 
            {
                Stream resource = typeof(TestResources).Assembly.GetManifestResourceStream(
                    typeof(TestResources).Namespace + ".google_message1.dat");

                Assert.NotNull(resource);

                byte[] bytes = new byte[resource.Length];
                int amtRead = resource.Read(bytes, 0, bytes.Length);
                Assert.AreEqual(bytes.Length, amtRead);
                return bytes;
            }
        }
        public static byte[] google_message2
        {
            get
            {
                Stream resource = typeof(TestResources).Assembly.GetManifestResourceStream(
                    typeof(TestResources).Namespace + ".google_message2.dat");

                Assert.NotNull(resource);
                byte[] bytes = new byte[resource.Length];
                int amtRead = resource.Read(bytes, 0, bytes.Length);
                Assert.AreEqual(bytes.Length, amtRead);
                return bytes;
            }
        }
    }
}
