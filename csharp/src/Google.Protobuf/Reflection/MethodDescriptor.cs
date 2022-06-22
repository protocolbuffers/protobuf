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

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a single method in a service.
    /// </summary>
    public sealed class MethodDescriptor : DescriptorBase
    {

        /// <value>
        /// The service this method belongs to.
        /// </value>
        public ServiceDescriptor Service { get; }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor InputType { get; private set; }

        /// <value>
        /// The method's input type.
        /// </value>
        public MessageDescriptor OutputType { get; private set; }

        /// <value>
        /// Indicates if client streams multiple requests.
        /// </value>
        public bool IsClientStreaming => Proto.ClientStreaming;

        /// <value>
        /// Indicates if server streams multiple responses.
        /// </value>
        public bool IsServerStreaming => Proto.ServerStreaming;

        /// <summary>
        /// The (possibly empty) set of custom options for this method.
        /// </summary>
        [Obsolete("CustomOptions are obsolete. Use the GetOptions() method.")]
        public CustomOptions CustomOptions => new CustomOptions(Proto.Options?._extensions?.ValuesByNumber);

        /// <summary>
        /// The <c>MethodOptions</c>, defined in <c>descriptor.proto</c>.
        /// If the options message is not present (i.e. there are no options), <c>null</c> is returned.
        /// Custom options can be retrieved as extensions of the returned message.
        /// NOTE: A defensive copy is created each time this property is retrieved.
        /// </summary>
        public MethodOptions GetOptions() => Proto.Options?.Clone();

        /// <summary>
        /// Gets a single value method option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public T GetOption<T>(Extension<MethodOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> c ? c.Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value method option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public RepeatedField<T> GetOption<T>(RepeatedExtension<MethodOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }

        internal MethodDescriptor(MethodDescriptorProto proto, FileDescriptor file,
                                  ServiceDescriptor parent, int index)
            : base(file, parent.FullName + "." + proto.Name, index)
        {
            Proto = proto;
            Service = parent;
            file.DescriptorPool.AddSymbol(this);
        }

        internal MethodDescriptorProto Proto { get; private set; }

        /// <summary>
        /// Returns a clone of the underlying <see cref="MethodDescriptorProto"/> describing this method.
        /// Note that a copy is taken every time this method is called, so clients using it frequently
        /// (and not modifying it) may want to cache the returned value.
        /// </summary>
        /// <returns>A protobuf representation of this method descriptor.</returns>
        public MethodDescriptorProto ToProto() => Proto.Clone();

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name => Proto.Name;

        internal void CrossLink()
        {
            IDescriptor lookup = File.DescriptorPool.LookupSymbol(Proto.InputType, this);
            if (lookup is not MessageDescriptor inpoutType)
            {
                throw new DescriptorValidationException(this, "\"" + Proto.InputType + "\" is not a message type.");
            }
            InputType = inpoutType;

            lookup = File.DescriptorPool.LookupSymbol(Proto.OutputType, this);
            if (lookup is not MessageDescriptor outputType)
            {
                throw new DescriptorValidationException(this, "\"" + Proto.OutputType + "\" is not a message type.");
            }
            OutputType = outputType;
        }
    }
}
 