using System.IO;

namespace Google.Protobuf
{
    /// <summary>
    /// Extension methods on <see cref="IMessage"/> and <see cref="IMessage{T}"/>.
    /// </summary>
    public static class MessageExtensions
    {
        public static void MergeFrom(this IMessage message, byte[] data)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(data, "data");
            CodedInputStream input = CodedInputStream.CreateInstance(data);
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, ByteString data)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(data, "data");
            CodedInputStream input = data.CreateCodedInput();
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, Stream input)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(input, "input");
            CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
            message.MergeFrom(codedInput);
            codedInput.CheckLastTagWas(0);
        }

        public static void MergeDelimitedFrom(this IMessage message, Stream input)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(input, "input");
            int size = (int) CodedInputStream.ReadRawVarint32(input);
            Stream limitedStream = new LimitedInputStream(input, size);
            message.MergeFrom(limitedStream);
        }

        public static byte[] ToByteArray(this IMessage message)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            byte[] result = new byte[message.CalculateSize()];
            CodedOutputStream output = CodedOutputStream.CreateInstance(result);
            message.WriteTo(output);
            output.CheckNoSpaceLeft();
            return result;
        }

        public static void WriteTo(this IMessage message, Stream output)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(output, "output");
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static void WriteDelimitedTo(this IMessage message, Stream output)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            ThrowHelper.ThrowIfNull(output, "output");
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            codedOutput.WriteRawVarint32((uint)message.CalculateSize());
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static ByteString ToByteString(this IMessage message)
        {
            ThrowHelper.ThrowIfNull(message, "message");
            return ByteString.AttachBytes(message.ToByteArray());
        }        
    }
}
