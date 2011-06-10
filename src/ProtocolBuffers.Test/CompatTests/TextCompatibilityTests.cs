using System.ComponentModel;
using System.IO;
using NUnit.Framework;

namespace Google.ProtocolBuffers.CompatTests
{
    [TestFixture]
    public class TextCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter text = new StringWriter();
            message.PrintTo(text);
            return text.ToString();
        }

        protected override TBuilder DeerializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            TextFormat.Merge(new StringReader((string)message), registry, (IBuilder)builder);
            return builder;
        }

        [Test, Explicit, Description("This test can take a very long time to run.")]
        public override void RoundTripMessage2OptimizeSize()
        {
            base.RoundTripMessage2OptimizeSize();
        }

        [Test, Explicit, Description("This test can take a very long time to run.")]
        public override void RoundTripMessage2OptimizeSpeed()
        {
            base.RoundTripMessage2OptimizeSpeed();
        }
    }
}