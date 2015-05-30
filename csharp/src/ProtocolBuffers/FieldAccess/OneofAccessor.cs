// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// Author: jieluo@google.com (Jie Luo)
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
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Access for an oneof
    /// </summary>
    internal class OneofAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly Func<TMessage, object> caseDelegate;
        private readonly Func<TBuilder, IBuilder> clearDelegate;
        private MessageDescriptor descriptor;

        internal OneofAccessor(MessageDescriptor descriptor, string name) 
        {
            this.descriptor = descriptor;
            MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
            PropertyInfo caseProperty = typeof(TMessage).GetProperty(name + "Case");
            if (clearMethod == null || caseProperty == null)
            {
                throw new ArgumentException("Not all required properties/methods available for oneof");
            }
            

            clearDelegate = ReflectionUtil.CreateDelegateFunc<TBuilder, IBuilder>(clearMethod);
            caseDelegate = ReflectionUtil.CreateUpcastDelegate<TMessage>(caseProperty.GetGetMethod());
        }

        /// <summary>
        /// Indicates whether the specified message has set any field in the oneof.
        /// </summary>
        public bool Has(TMessage message)
        {
            return ((int) caseDelegate(message) != 0);
        }

        /// <summary>
        /// Clears the oneof in the specified builder.
        /// </summary>
        public void Clear(TBuilder builder)
        {
            clearDelegate(builder);
        }

        /// <summary>
        /// Indicates which field in the oneof is set for specified message
        /// </summary>
        public virtual FieldDescriptor GetOneofFieldDescriptor(TMessage message)
        {
            int fieldNumber = (int) caseDelegate(message);
            if (fieldNumber > 0)
            {
                return descriptor.FindFieldByNumber(fieldNumber);
            }
            return null;
        }
    }
}
