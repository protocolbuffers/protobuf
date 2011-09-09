using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers;
using System.IO;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Extensions for the IRpcServerStub
    /// </summary>
    public static class ServiceExtensions
    {
        /// <summary>
        /// Used to implement a service endpoint on an HTTP server.  This works with services generated with the
        /// service_generator_type option set to IRPCDISPATCH.
        /// </summary>
        /// <param name="stub">The service execution stub</param>
        /// <param name="methodName">The name of the method being invoked</param>
        /// <param name="options">optional arguments for the format reader/writer</param>
        /// <param name="contentType">The mime type for the input stream</param>
        /// <param name="input">The input stream</param>
        /// <param name="responseType">The mime type for the output stream</param>
        /// <param name="output">The output stream</param>
        public static void HttpCallMethod(this IRpcServerStub stub, string methodName, MessageFormatOptions options, 
            string contentType, Stream input, string responseType, Stream output)
        {
            ICodedInputStream codedInput = MessageFormatFactory.CreateInputStream(options, contentType, input);
            IMessageLite response = stub.CallMethod(methodName, codedInput, options.ExtensionRegistry);
            response.WriteTo(options, responseType, output);
        }
    }
}
