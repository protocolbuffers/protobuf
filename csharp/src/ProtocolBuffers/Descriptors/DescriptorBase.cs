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
    /// Base class for nearly all descriptors, providing common functionality.
    /// </summary>
    /// <typeparam name="TProto">Type of the protocol buffer form of this descriptor</typeparam>
    /// <typeparam name="TOptions">Type of the options protocol buffer for this descriptor</typeparam>
    public abstract class DescriptorBase<TProto, TOptions> : IDescriptor<TProto>
        where TProto : IMessage, IDescriptorProto<TOptions>
    {
        private TProto proto;
        private readonly FileDescriptor file;
        private readonly string fullName;

        protected DescriptorBase(TProto proto, FileDescriptor file, string fullName)
        {
            this.proto = proto;
            this.file = file;
            this.fullName = fullName;
        }

        internal virtual void ReplaceProto(TProto newProto)
        {
            this.proto = newProto;
        }

        protected static string ComputeFullName(FileDescriptor file, MessageDescriptor parent, string name)
        {
            if (parent != null)
            {
                return parent.FullName + "." + name;
            }
            if (file.Package.Length > 0)
            {
                return file.Package + "." + name;
            }
            return name;
        }

        IMessage IDescriptor.Proto
        {
            get { return proto; }
        }

        /// <summary>
        /// Returns the protocol buffer form of this descriptor.
        /// </summary>
        public TProto Proto
        {
            get { return proto; }
        }

        public TOptions Options
        {
            get { return proto.Options; }
        }

        /// <summary>
        /// The fully qualified name of the descriptor's target.
        /// </summary>
        public string FullName
        {
            get { return fullName; }
        }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public string Name
        {
            get { return proto.Name; }
        }

        /// <value>
        /// The file this descriptor was declared in.
        /// </value>
        public FileDescriptor File
        {
            get { return file; }
        }
    }
}