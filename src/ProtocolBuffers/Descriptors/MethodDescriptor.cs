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
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Describes a single method in a service.
    /// </summary>
    public sealed class MethodDescriptor : IndexedDescriptorBase<MethodDescriptorProto, MethodOptions>
    {
        private readonly ServiceDescriptor service;
        private MessageDescriptor inputType;
        private MessageDescriptor outputType;

        /// <value>
        /// The service this method belongs to.
        /// </value>
        public ServiceDescriptor Service
        {
            get { return service; }
        }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor InputType
        {
            get { return inputType; }
        }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor OutputType
        {
            get { return outputType; }
        }

        internal MethodDescriptor(MethodDescriptorProto proto, FileDescriptor file,
                                  ServiceDescriptor parent, int index)
            : base(proto, file, parent.FullName + "." + proto.Name, index)
        {
            service = parent;
            file.DescriptorPool.AddSymbol(this);
        }

        internal void CrossLink()
        {
            IDescriptor lookup = File.DescriptorPool.LookupSymbol(Proto.InputType, this);
            if (!(lookup is MessageDescriptor))
            {
                throw new DescriptorValidationException(this, "\"" + Proto.InputType + "\" is not a message type.");
            }
            inputType = (MessageDescriptor) lookup;

            lookup = File.DescriptorPool.LookupSymbol(Proto.OutputType, this);
            if (!(lookup is MessageDescriptor))
            {
                throw new DescriptorValidationException(this, "\"" + Proto.OutputType + "\" is not a message type.");
            }
            outputType = (MessageDescriptor) lookup;
        }
    }
}