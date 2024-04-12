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

using Google.Protobuf.Collections;
using System;
using System.Collections.Generic;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a service type.
    /// </summary>
    public sealed class ServiceDescriptor : DescriptorBase
    {
        internal ServiceDescriptor(ServiceDescriptorProto proto, FileDescriptor file, int index)
            : base(file, file.ComputeFullName(null, proto.Name), index)
        {
            Proto = proto;
            Methods = DescriptorUtil.ConvertAndMakeReadOnly(proto.Method,
                                                            (method, i) => new MethodDescriptor(method, file, this, i));

            file.DescriptorPool.AddSymbol(this);
        }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name => Proto.Name;

        internal override IReadOnlyList<DescriptorBase> GetNestedDescriptorListForField(int fieldNumber) =>
            fieldNumber switch
            {
                ServiceDescriptorProto.MethodFieldNumber => (IReadOnlyList<DescriptorBase>)Methods,
                _ => null,
            };

        internal ServiceDescriptorProto Proto { get; }

        /// <summary>
        /// Returns a clone of the underlying <see cref="ServiceDescriptorProto"/> describing this service.
        /// Note that a copy is taken every time this method is called, so clients using it frequently
        /// (and not modifying it) may want to cache the returned value.
        /// </summary>
        /// <returns>A protobuf representation of this service descriptor.</returns>
        public ServiceDescriptorProto ToProto() => Proto.Clone();

        /// <value>
        /// An unmodifiable list of methods in this service.
        /// </value>
        public IList<MethodDescriptor> Methods { get; }

        /// <summary>
        /// Finds a method by name.
        /// </summary>
        /// <param name="name">The unqualified name of the method (e.g. "Foo").</param>
        /// <returns>The method's descriptor, or null if not found.</returns>
        public MethodDescriptor FindMethodByName(string name) =>
            File.DescriptorPool.FindSymbol<MethodDescriptor>(FullName + "." + name);

        /// <summary>
        /// The (possibly empty) set of custom options for this service.
        /// </summary>
        [Obsolete("CustomOptions are obsolete. Use the GetOptions() method.")]
        public CustomOptions CustomOptions => new CustomOptions(Proto.Options?._extensions?.ValuesByNumber);

        /// <summary>
        /// The <c>ServiceOptions</c>, defined in <c>descriptor.proto</c>.
        /// If the options message is not present (i.e. there are no options), <c>null</c> is returned.
        /// Custom options can be retrieved as extensions of the returned message.
        /// NOTE: A defensive copy is created each time this property is retrieved.
        /// </summary>
        public ServiceOptions GetOptions() => Proto.Options?.Clone();

        /// <summary>
        /// Gets a single value service option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public T GetOption<T>(Extension<ServiceOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> ? (value as IDeepCloneable<T>).Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value service option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public RepeatedField<T> GetOption<T>(RepeatedExtension<ServiceOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }

        internal void CrossLink()
        {
            foreach (MethodDescriptor method in Methods)
            {
                method.CrossLink();
            }
        }
    }
}
 