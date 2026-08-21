#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
            // cross-linked yet. But by the time the delegates are called by user code, all will be
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
