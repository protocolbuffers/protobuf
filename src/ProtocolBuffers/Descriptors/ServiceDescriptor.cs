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
using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Describes a service type.
    /// </summary>
    public sealed class ServiceDescriptor : IndexedDescriptorBase<ServiceDescriptorProto, ServiceOptions>
    {
        private readonly IList<MethodDescriptor> methods;

        public ServiceDescriptor(ServiceDescriptorProto proto, FileDescriptor file, int index)
            : base(proto, file, ComputeFullName(file, null, proto.Name), index)
        {
            methods = DescriptorUtil.ConvertAndMakeReadOnly(proto.MethodList,
                                                            (method, i) => new MethodDescriptor(method, file, this, i));

            file.DescriptorPool.AddSymbol(this);
        }

        /// <value>
        /// An unmodifiable list of methods in this service.
        /// </value>
        public IList<MethodDescriptor> Methods
        {
            get { return methods; }
        }

        /// <summary>
        /// Finds a method by name.
        /// </summary>
        /// <param name="name">The unqualified name of the method (e.g. "Foo").</param>
        /// <returns>The method's decsriptor, or null if not found.</returns>
        public MethodDescriptor FindMethodByName(String name)
        {
            return File.DescriptorPool.FindSymbol<MethodDescriptor>(FullName + "." + name);
        }

        internal void CrossLink()
        {
            foreach (MethodDescriptor method in methods)
            {
                method.CrossLink();
            }
        }

        internal override void ReplaceProto(ServiceDescriptorProto newProto)
        {
            base.ReplaceProto(newProto);
            for (int i = 0; i < methods.Count; i++)
            {
                methods[i].ReplaceProto(newProto.GetMethod(i));
            }
        }
    }
}