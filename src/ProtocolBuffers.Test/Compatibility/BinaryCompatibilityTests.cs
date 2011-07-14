using System;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Compatibility
{
    [TestFixture]
    public class BinaryCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            byte[] bresult = message.ToByteArray();
            return bresult;
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            return builder.MergeFrom((byte[])message, registry);
        }
    }
}