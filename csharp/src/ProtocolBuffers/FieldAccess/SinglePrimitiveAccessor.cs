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
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Access for a non-repeated field of a "primitive" type (i.e. not another message or an enum).
    /// </summary>
    internal class SinglePrimitiveAccessor<TMessage, TBuilder> : IFieldAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly Type clrType;
        private readonly Func<TMessage, object> getValueDelegate;
        private readonly Action<TBuilder, object> setValueDelegate;
        private readonly Func<TMessage, bool> hasDelegate;
        private readonly Func<TBuilder, IBuilder> clearDelegate;

        /// <summary>
        /// The CLR type of the field (int, the enum type, ByteString, the message etc).
        /// As declared by the property.
        /// </summary>
        protected Type ClrType
        {
            get { return clrType; }
        }

        internal SinglePrimitiveAccessor(FieldDescriptor fieldDescriptor, string name, bool supportFieldPresence)
        {
            PropertyInfo messageProperty = typeof(TMessage).GetProperty(name, null, ReflectionUtil.EmptyTypes);
            PropertyInfo builderProperty = typeof(TBuilder).GetProperty(name, null, ReflectionUtil.EmptyTypes);
            MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
            if (messageProperty == null || builderProperty == null || clearMethod == null)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }

            if (supportFieldPresence)
            {
                PropertyInfo hasProperty = typeof(TMessage).GetProperty("Has" + name);
                if (hasProperty == null)
                {
                    throw new ArgumentException("Has properties not available");
                }
                hasDelegate = ReflectionUtil.CreateDelegateFunc<TMessage, bool>(hasProperty.GetGetMethod());
            } else
            {
                hasDelegate = message => !GetValue(message).Equals(fieldDescriptor.DefaultValue);
            }

            clrType = messageProperty.PropertyType;
            clearDelegate = ReflectionUtil.CreateDelegateFunc<TBuilder, IBuilder>(clearMethod);
            getValueDelegate = ReflectionUtil.CreateUpcastDelegate<TMessage>(messageProperty.GetGetMethod());
            setValueDelegate = ReflectionUtil.CreateDowncastDelegate<TBuilder>(builderProperty.GetSetMethod());
        }

        public bool Has(TMessage message)
        {
            return hasDelegate(message);
        }

        public void Clear(TBuilder builder)
        {
            clearDelegate(builder);
        }

        /// <summary>
        /// Only valid for message types - this implementation throws InvalidOperationException.
        /// </summary>
        public virtual IBuilder CreateBuilder()
        {
            throw new InvalidOperationException();
        }

        public virtual object GetValue(TMessage message)
        {
            return getValueDelegate(message);
        }

        public virtual void SetValue(TBuilder builder, object value)
        {
            setValueDelegate(builder, value);
        }

        #region Methods only related to repeated values

        public int GetRepeatedCount(TMessage message)
        {
            throw new InvalidOperationException();
        }

        public object GetRepeatedValue(TMessage message, int index)
        {
            throw new InvalidOperationException();
        }

        public void SetRepeated(TBuilder builder, int index, object value)
        {
            throw new InvalidOperationException();
        }

        public void AddRepeated(TBuilder builder, object value)
        {
            throw new InvalidOperationException();
        }

        public object GetRepeatedWrapper(TBuilder builder)
        {
            throw new InvalidOperationException();
        }

        #endregion
    }
}