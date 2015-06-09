using System;
using System.IO;
using Google.Protobuf;

namespace Google.Protobuf
{
    public sealed class MessageParser<T> where T : IMessage<T>
    {
        private readonly Func<T> factory; 

        public MessageParser(Func<T> factory)
        {
            this.factory = factory;
        }

        // Creates a template instance ready for population.
        internal T CreateTemplate()
        {
            return factory();
        }

        public T ParseFrom(byte[] data)
        {
            T message = factory();
            message.MergeFrom(data);
            return message;
        }

        public T ParseFrom(ByteString data)
        {
            T message = factory();
            message.MergeFrom(data);
            return message;
        }

        public T ParseFrom(Stream input)
        {
            T message = factory();
            message.MergeFrom(input);
            return message;
        }

        public T ParseDelimitedFrom(Stream input)
        {
            T message = factory();
            message.MergeDelimitedFrom(input);
            return message;
        }

        public T ParseFrom(ICodedInputStream input)
        {
            T message = factory();
            message.MergeFrom(input);
            return message;
        }
    }
}
