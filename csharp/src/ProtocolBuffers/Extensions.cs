using System.IO;

namespace Google.Protobuf
{
    // TODO: MessageExtensions?
    public static class Extensions
    {
        public static void MergeFrom(this IMessage message, byte[] data)
        {
            CodedInputStream input = CodedInputStream.CreateInstance(data);
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, ByteString data)
        {
            CodedInputStream input = data.CreateCodedInput();
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, Stream input)
        {
            CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
            message.MergeFrom(codedInput);
            codedInput.CheckLastTagWas(0);
        }

        public static void MergeDelimitedFrom(this IMessage message, Stream input)
        {
            int size = (int)CodedInputStream.ReadRawVarint32(input);
            Stream limitedStream = new LimitedInputStream(input, size);
            message.MergeFrom(limitedStream);
        }

        public static byte[] ToByteArray(this IMessage message)
        {
            byte[] result = new byte[message.CalculateSize()];
            CodedOutputStream output = CodedOutputStream.CreateInstance(result);
            message.WriteTo(output);
            output.CheckNoSpaceLeft();
            return result;
        }

        public static void WriteTo(this IMessage message, Stream output)
        {
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static void WriteTo(this IMessage message, ICodedOutputStream output)
        {
            message.WriteTo(output);
        }

        public static void WriteDelimitedTo(this IMessage message, Stream output)
        {
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            codedOutput.WriteRawVarint32((uint)message.CalculateSize());
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static ByteString ToByteString(this IMessage message)
        {
            return ByteString.AttachBytes(message.ToByteArray());
        }        
    }
}
