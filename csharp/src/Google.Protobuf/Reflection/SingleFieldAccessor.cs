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
    /// Accessor for single fields.
    /// </summary>
    internal sealed class SingleFieldAccessor : FieldAccessorBase
    {
        // All the work here is actually done in the constructor - it creates the appropriate delegates.
        // There are various cases to consider, based on the property type (message, string/bytes, or "genuine" primitive)
        // and proto2 vs proto3 for non-message types, as proto3 doesn't support "full" presence detection or default
        // values.

        private readonly Action<IMessage, object> setValueDelegate;
        private readonly Action<IMessage> clearDelegate;
        private readonly Func<IMessage, bool> hasDelegate;

        internal SingleFieldAccessor(PropertyInfo property, FieldDescriptor descriptor) : base(property, descriptor)
        {
            if (!property.CanWrite)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            setValueDelegate = ReflectionUtil.CreateActionIMessageObject(property.GetSetMethod());
            if (descriptor.File.Proto.Syntax == "proto2")
            {
                MethodInfo hasMethod = property.DeclaringType.GetRuntimeProperty("Has" + property.Name).GetMethod;
                hasDelegate = ReflectionUtil.CreateFuncIMessageBool(hasMethod ?? throw new ArgumentException("Not all required properties/methods are available"));
                MethodInfo clearMethod = property.DeclaringType.GetRuntimeMethod("Clear" + property.Name, ReflectionUtil.EmptyTypes);
                clearDelegate = ReflectionUtil.CreateActionIMessage(clearMethod ?? throw new ArgumentException("Not all required properties/methods are available"));
            }
            else
            {
                hasDelegate = (_) => throw new InvalidOperationException("HasValue is not implemented for proto3 fields"); var clrType = property.PropertyType;

                // TODO: Validate that this is a reasonable single field? (Should be a value type, a message type, or string/ByteString.) 
                object defaultValue =
                    descriptor.FieldType == FieldType.Message ? null
                    : clrType == typeof(string) ? ""
                    : clrType == typeof(ByteString) ? ByteString.Empty
                    : Activator.CreateInstance(clrType);
                clearDelegate = message => SetValue(message, defaultValue);
            }
        }

        public override void Clear(IMessage message)
        {
            clearDelegate(message);
        }

        public override bool HasValue(IMessage message)
        {
            return hasDelegate(message);
        }

        public override void SetValue(IMessage message, object value)
        {
            setValueDelegate(message, value);
        }
    }
}
