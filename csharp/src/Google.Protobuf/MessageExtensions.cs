#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

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
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(data, "data");
            CodedInputStream input = CodedInputStream.CreateInstance(data);
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, ByteString data)
        {
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(data, "data");
            CodedInputStream input = data.CreateCodedInput();
            message.MergeFrom(input);
            input.CheckLastTagWas(0);
        }

        public static void MergeFrom(this IMessage message, Stream input)
        {
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(input, "input");
            CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
            message.MergeFrom(codedInput);
            codedInput.CheckLastTagWas(0);
        }

        public static void MergeDelimitedFrom(this IMessage message, Stream input)
        {
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(input, "input");
            int size = (int) CodedInputStream.ReadRawVarint32(input);
            Stream limitedStream = new LimitedInputStream(input, size);
            message.MergeFrom(limitedStream);
        }

        public static byte[] ToByteArray(this IMessage message)
        {
            Preconditions.CheckNotNull(message, "message");
            byte[] result = new byte[message.CalculateSize()];
            CodedOutputStream output = CodedOutputStream.CreateInstance(result);
            message.WriteTo(output);
            output.CheckNoSpaceLeft();
            return result;
        }

        public static void WriteTo(this IMessage message, Stream output)
        {
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static void WriteDelimitedTo(this IMessage message, Stream output)
        {
            Preconditions.CheckNotNull(message, "message");
            Preconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            codedOutput.WriteRawVarint32((uint)message.CalculateSize());
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        public static ByteString ToByteString(this IMessage message)
        {
            Preconditions.CheckNotNull(message, "message");
            return ByteString.AttachBytes(message.ToByteArray());
        }        
    }
}
