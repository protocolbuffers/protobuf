using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers.Compatibility
{
    [TestClass]
    public class BinaryCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            byte[] bresult = message.ToByteArray();
            return Convert.ToBase64String(bresult);
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            return builder.MergeFrom((byte[])Convert.FromBase64String((string)message), registry);
        }
    }
}