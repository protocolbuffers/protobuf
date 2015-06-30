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
using Google.Protobuf.Descriptors;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Accessor for single fields.
    /// </summary>
    /// <typeparam name="T">The type of message containing the field.</typeparam>
    internal sealed class SingleFieldAccessor<T> : FieldAccessorBase<T> where T : IMessage<T>
    {
        // All the work here is actually done in the constructor - it creates the appropriate delegates.
        // There are various cases to consider, based on the property type (message, string/bytes, or "genuine" primitive)
        // and proto2 vs proto3 for non-message types, as proto3 doesn't support "full" presence detection or default
        // values.

        private readonly Action<T, object> setValueDelegate;
        private readonly Action<T> clearDelegate;
        private readonly Func<T, bool> hasValueDelegate;

        internal SingleFieldAccessor(FieldDescriptor descriptor, string name, bool supportsFieldPresence) : base(name)
        {
            PropertyInfo property = typeof(T).GetProperty(name);
            // We know there *is* such a property, or the base class constructor would have thrown. We should be able to write
            // to it though.
            if (!property.CanWrite)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            setValueDelegate = ReflectionUtil.CreateDowncastDelegate<T>(property.GetSetMethod());

            var clrType = property.PropertyType;

            if (typeof(IMessage).IsAssignableFrom(clrType))
            {
                // Message types are simple - we only need to detect nullity.
                clearDelegate = message => SetValue(message, null);
                hasValueDelegate = message => GetValue(message) == null;
            }

            if (supportsFieldPresence)
            {
                // Proto2: we expect a HasFoo property and a ClearFoo method.
                // For strings and byte arrays, setting the property to null would have the equivalent effect,
                // but we generate the method for consistency, which makes this simpler.
                PropertyInfo hasProperty = typeof(T).GetProperty("Has" + name);
                MethodInfo clearMethod = typeof(T).GetMethod("Clear" + name);
                if (hasProperty == null || clearMethod == null || !hasProperty.CanRead)
                {
                    throw new ArgumentException("Not all required properties/methods available");
                }
                hasValueDelegate = ReflectionUtil.CreateDelegateFunc<T, bool>(hasProperty.GetGetMethod());
                clearDelegate = ReflectionUtil.CreateDelegateAction<T>(clearMethod);
            }
            else
            {
                /*
                // TODO(jonskeet): Reimplement. We need a better way of working out default values.
                // Proto3: for field detection, we just need the default value of the field (0, "", byte[0] etc)
                // To clear a field, we set the value to that default.
                object defaultValue = descriptor.DefaultValue;
                hasValueDelegate = message => GetValue(message).Equals(defaultValue);
                clearDelegate = message => SetValue(message, defaultValue);
                */
            }
        }

        public override bool HasValue(T message)
        {
            return hasValueDelegate(message);
        }

        public override void Clear(T message)
        {
            clearDelegate(message);
        }

        public override void SetValue(T message, object value)
        {
            setValueDelegate(message, value);
        }
    }
}
