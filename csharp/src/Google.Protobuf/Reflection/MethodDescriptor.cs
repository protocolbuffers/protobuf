#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a single method in a service.
    /// </summary>
    public sealed class MethodDescriptor : DescriptorBase
    {
        private readonly MethodDescriptorProto proto;
        private readonly ServiceDescriptor service;
        private MessageDescriptor inputType;
        private MessageDescriptor outputType;

        /// <value>
        /// The service this method belongs to.
        /// </value>
        public ServiceDescriptor Service { get { return service; } }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor InputType { get { return inputType; } }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor OutputType { get { return outputType; } }

        /// <value>
        /// Indicates if client streams multiple requests.
        /// </value>
        public bool IsClientStreaming { get { return proto.ClientStreaming; } }

        /// <value>
        /// Indicates if server streams multiple responses.
        /// </value>
        public bool IsServerStreaming { get { return proto.ServerStreaming; } }

        /// <summary>
        /// The (possibly empty) set of custom options for this method.
        /// </summary>
        public CustomOptions CustomOptions => Proto.Options?.CustomOptions ?? CustomOptions.Empty;

        internal MethodDescriptor(MethodDescriptorProto proto, FileDescriptor file,
                                  ServiceDescriptor parent, int index)
            : base(file, parent.FullName + "." + proto.Name, index)
        {
            this.proto = proto;
            service = parent;
            file.DescriptorPool.AddSymbol(this);
        }

        internal MethodDescriptorProto Proto { get { return proto; } }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name { get { return proto.Name; } }

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