using System.ComponentModel;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers.Compatibility
{
    [TestClass]
    public class TextCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter text = new StringWriter();
            message.PrintTo(text);
            return text.ToString();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            TextFormat.Merge(new StringReader((string)message), registry, (IBuilder)builder);
            return builder;
        }

        [TestMethod, System.ComponentModel.Description("This test can take a very long time to run.")]
        public override void RoundTripMessage2OptimizeSize()
        {
            //base.RoundTripMessage2OptimizeSize();
        }

        [TestMethod, System.ComponentModel.Description("This test can take a very long time to run.")]
        public override void RoundTripMessage2OptimizeSpeed()
        {
            //base.RoundTripMessage2OptimizeSpeed();
        }
    }
}