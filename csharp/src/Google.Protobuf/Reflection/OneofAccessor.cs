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

using System;
using System.Reflection;
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Reflection access for a oneof, allowing clear and "get case" actions.
    /// </summary>
    public sealed class OneofAccessor
    {
        private readonly Func<IMessage, int> caseDelegate;
        private readonly Action<IMessage> clearDelegate;

        private OneofAccessor(OneofDescriptor descriptor, Func<IMessage, int> caseDelegate, Action<IMessage> clearDelegate)
        {
            Descriptor = descriptor;
            this.caseDelegate = caseDelegate;
            this.clearDelegate = clearDelegate;
        }

        internal static OneofAccessor ForRegularOneof(
            OneofDescriptor descriptor,
            PropertyInfo caseProperty,
            MethodInfo clearMethod) =>
            new OneofAccessor(
                descriptor,
                ReflectionUtil.CreateFuncIMessageInt32(caseProperty.GetGetMethod()),
                ReflectionUtil.CreateActionIMessage(clearMethod));

        internal static OneofAccessor ForSyntheticOneof(OneofDescriptor descriptor)
        {
            // Note: descriptor.Fields will be null when this method is called, because we haven't
            // cross-linked yet. But by the time the delgates are called by user code, all will be
            // well. (That's why we capture the descriptor itself rather than a field.)
            return new OneofAccessor(descriptor,
                message => descriptor.Fields[0].Accessor.HasValue(message) ? descriptor.Fields[0].FieldNumber : 0,
                message => descriptor.Fields[0].Accessor.Clear(message));
        }

        /// <summary>
        /// Gets the descriptor for this oneof.
        /// </summary>
        /// <value>
        /// The descriptor of the oneof.
        /// </value>
        public OneofDescriptor Descriptor { get; }

        /// <summary>
        /// Clears the oneof in the specified message.
        /// </summary>
        public void Clear(IMessage message) => clearDelegate(message);

        /// <summary>
        /// Indicates which field in the oneof is set for specified message
        /// </summary>
        public FieldDescriptor GetCaseFieldDescriptor(IMessage message)
        {
            int fieldNumber = caseDelegate(message);
            return fieldNumber > 0
                ? Descriptor.ContainingType.FindFieldByNumber(fieldNumber)
                : null;
        }
    }
}
