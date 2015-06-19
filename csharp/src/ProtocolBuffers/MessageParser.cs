using System;
using System.IO;

namespace Google.Protobuf
{
    /// <summary>
    /// A parser for a specific message type.
    /// </summary>
    /// <remarks>
    /// <p>
    /// This delegates most behavior to the
    /// <see cref="IMessage.MergeFrom"/> implementation within the original type, but
    /// provides convenient overloads to parse from a variety of sources.
    /// </p>
    /// <p>
    /// Most applications will never need to create their own instances of this type;
    /// instead, use the static <c>Parser</c> property of a generated message type to obtain a
    /// parser for that type.
    /// </p>
    /// </remarks>
    /// <typeparam name="T">The type of message to be parsed.</typeparam>
    public sealed class MessageParser<T> where T : IMessage<T>
    {
        private readonly Func<T> factory; 

        /// <summary>
        /// Creates a new parser.
        /// </summary>
        /// <remarks>
        /// The factory method is effectively an optimization over using a generic constraint
        /// to require a parameterless constructor: delegates are significantly faster to execute.
        /// </remarks>
        /// <param name="factory">Function to invoke when a new, empty message is required.</param>
        public MessageParser(Func<T> factory)
        {
            this.factory = factory;
        }

        /// <summary>
        /// Creates a template instance ready for population.
        /// </summary>
        /// <returns>An empty message.</returns>
        internal T CreateTemplate()
        {
            return factory();
        }

        /// <summary>
        /// Parses a message from a byte array.
        /// </summary>
        /// <param name="data">The byte array containing the message. Must not be null.</param>
        /// <returns>The newly parsed message.</returns>
        public T ParseFrom(byte[] data)
        {
            ThrowHelper.ThrowIfNull(data, "data");
            T message = factory();
            message.MergeFrom(data);
            return message;
        }

        public T ParseFrom(ByteString data)
        {
            ThrowHelper.ThrowIfNull(data, "data");
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

        public T ParseFrom(CodedInputStream input)
        {
            T message = factory();
            message.MergeFrom(input);
            return message;
        }
    }
}
