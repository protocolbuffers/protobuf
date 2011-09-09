#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Implementation of the non-generic IMessage interface as far as possible.
    /// </summary>
    public abstract partial class AbstractMessageLite<TMessage, TBuilder> : IMessageLite<TMessage, TBuilder>
        where TMessage : AbstractMessageLite<TMessage, TBuilder>
        where TBuilder : AbstractBuilderLite<TMessage, TBuilder>
    {
        public abstract TBuilder CreateBuilderForType();

        public abstract TBuilder ToBuilder();

        public abstract TMessage DefaultInstanceForType { get; }

        public abstract bool IsInitialized { get; }

        public abstract void WriteTo(ICodedOutputStream output);

        public abstract int SerializedSize { get; }

        //public override bool Equals(object other) {
        //}

        //public override int GetHashCode() {
        //}

        public abstract void PrintTo(TextWriter writer);

        #region IMessageLite<TMessage,TBuilder> Members

        /// <summary>
        /// Serializes the message to a ByteString. This is a trivial wrapper
        /// around WriteTo(ICodedOutputStream).
        /// </summary>
        public ByteString ToByteString()
        {
            ByteString.CodedBuilder output = new ByteString.CodedBuilder(SerializedSize);
            WriteTo(output.CodedOutput);
            return output.Build();
        }

        /// <summary>
        /// Serializes the message to a byte array. This is a trivial wrapper
        /// around WriteTo(ICodedOutputStream).
        /// </summary>
        public byte[] ToByteArray()
        {
            byte[] result = new byte[SerializedSize];
            CodedOutputStream output = CodedOutputStream.CreateInstance(result);
            WriteTo(output);
            output.CheckNoSpaceLeft();
            return result;
        }

        /// <summary>
        /// Serializes the message and writes it to the given stream.
        /// This is just a wrapper around WriteTo(CodedOutputStream). This
        /// does not flush or close the stream.
        /// </summary>
        /// <param name="output"></param>
        public void WriteTo(Stream output)
        {
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            WriteTo(codedOutput);
            codedOutput.Flush();
        }

        /// <summary>
        /// Like WriteTo(Stream) but writes the size of the message as a varint before
        /// writing the data. This allows more data to be written to the stream after the
        /// message without the need to delimit the message data yourself. Use 
        /// IBuilder.MergeDelimitedFrom(Stream) or the static method
        /// YourMessageType.ParseDelimitedFrom(Stream) to parse messages written by this method.
        /// </summary>
        /// <param name="output"></param>
        public void WriteDelimitedTo(Stream output)
        {
            CodedOutputStream codedOutput = CodedOutputStream.CreateInstance(output);
            codedOutput.WriteRawVarint32((uint) SerializedSize);
            WriteTo(codedOutput);
            codedOutput.Flush();
        }

        IBuilderLite IMessageLite.WeakCreateBuilderForType()
        {
            return CreateBuilderForType();
        }

        IBuilderLite IMessageLite.WeakToBuilder()
        {
            return ToBuilder();
        }

        IMessageLite IMessageLite.WeakDefaultInstanceForType
        {
            get { return DefaultInstanceForType; }
        }

        #endregion
    }
}