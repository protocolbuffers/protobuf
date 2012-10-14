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
using System.Collections;
using System.Reflection;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Accessor for a repeated field of type int, ByteString etc.
    /// </summary>
    internal class RepeatedPrimitiveAccessor<TMessage, TBuilder> : IFieldAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly Type clrType;
        private readonly Func<TMessage, object> getValueDelegate;
        private readonly Func<TBuilder, IBuilder> clearDelegate;
        private readonly Action<TBuilder, object> addValueDelegate;
        private readonly Func<TBuilder, object> getRepeatedWrapperDelegate;
        private readonly Func<TMessage, int> countDelegate;
        private readonly MethodInfo getElementMethod;
        private readonly MethodInfo setElementMethod;

        // Replacement for Type.EmptyTypes which apparently isn't available on the compact framework
        internal static readonly Type[] EmptyTypes = new Type[0];

        /// <summary>
        /// The CLR type of the field (int, the enum type, ByteString, the message etc).
        /// This is taken from the return type of the method used to retrieve a single
        /// value.
        /// </summary>
        protected Type ClrType
        {
            get { return clrType; }
        }

        internal RepeatedPrimitiveAccessor(string name)
        {
            PropertyInfo messageProperty = typeof(TMessage).GetProperty(name + "List");
            PropertyInfo builderProperty = typeof(TBuilder).GetProperty(name + "List");
            PropertyInfo countProperty = typeof(TMessage).GetProperty(name + "Count");
            MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name, EmptyTypes);
            getElementMethod = typeof(TMessage).GetMethod("Get" + name, new Type[] {typeof(int)});
            clrType = getElementMethod.ReturnType;
            MethodInfo addMethod = typeof(TBuilder).GetMethod("Add" + name, new Type[] {ClrType});
            setElementMethod = typeof(TBuilder).GetMethod("Set" + name, new Type[] {typeof(int), ClrType});
            if (messageProperty == null
                || builderProperty == null
                || countProperty == null
                || clearMethod == null
                || addMethod == null
                || getElementMethod == null
                || setElementMethod == null)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            clearDelegate = ReflectionUtil.CreateDelegateFunc<TBuilder, IBuilder>(clearMethod);
            countDelegate = ReflectionUtil.CreateDelegateFunc<TMessage, int>(countProperty.GetGetMethod());
            getValueDelegate = ReflectionUtil.CreateUpcastDelegate<TMessage>(messageProperty.GetGetMethod());
            addValueDelegate = ReflectionUtil.CreateDowncastDelegateIgnoringReturn<TBuilder>(addMethod);
            getRepeatedWrapperDelegate = ReflectionUtil.CreateUpcastDelegate<TBuilder>(builderProperty.GetGetMethod());
        }

        public bool Has(TMessage message)
        {
            throw new InvalidOperationException();
        }

        public virtual IBuilder CreateBuilder()
        {
            throw new InvalidOperationException();
        }

        public virtual object GetValue(TMessage message)
        {
            return getValueDelegate(message);
        }

        public void SetValue(TBuilder builder, object value)
        {
            // Add all the elements individually.  This serves two purposes:
            // 1) Verifies that each element has the correct type.
            // 2) Insures that the caller cannot modify the list later on and
            //    have the modifications be reflected in the message.
            Clear(builder);
            foreach (object element in (IEnumerable) value)
            {
                AddRepeated(builder, element);
            }
        }

        public void Clear(TBuilder builder)
        {
            clearDelegate(builder);
        }

        public int GetRepeatedCount(TMessage message)
        {
            return countDelegate(message);
        }

        public virtual object GetRepeatedValue(TMessage message, int index)
        {
            return getElementMethod.Invoke(message, new object[] {index});
        }

        public virtual void SetRepeated(TBuilder builder, int index, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            setElementMethod.Invoke(builder, new object[] {index, value});
        }

        public virtual void AddRepeated(TBuilder builder, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            addValueDelegate(builder, value);
        }

        /// <summary>
        /// The builder class's accessor already builds a read-only wrapper for
        /// us, which is exactly what we want.
        /// </summary>
        public object GetRepeatedWrapper(TBuilder builder)
        {
            return getRepeatedWrapperDelegate(builder);
        }
    }
}