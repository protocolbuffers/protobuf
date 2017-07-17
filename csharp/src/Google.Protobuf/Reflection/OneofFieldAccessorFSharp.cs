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
    internal sealed class OneofFieldAccessorFSharp : IFieldAccessor
    {

        private readonly FieldDescriptor descriptor;
        private readonly Func<IMessage, object> getValueDelegate;
        private readonly Action<IMessage, object> setValueDelegate;
        private readonly Action<IMessage> clearDelegate;

        internal OneofFieldAccessorFSharp(FieldDescriptor descriptor, string clrName)
        {
            var oneofAccessor = descriptor.ContainingOneof.Accessor;
            var oneofProperty = oneofAccessor.GetCaseProperty();
            if (!oneofProperty.CanWrite)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            this.descriptor = descriptor;

            
            // Get inner case item
            var duCaseType = Type.GetType(GetDuFullTypeName(oneofProperty, clrName));
            var innerGetter = duCaseType.GetProperty("Item").GetMethod;
            getValueDelegate = ReflectionUtil.CreateFuncIMessageT<object>(oneofProperty.GetMethod, innerGetter);

            // Wrap value on DU case and set
            setValueDelegate = ReflectionUtil.CreateActionIMessageObject(
                oneofProperty.PropertyType.GetMethod("New" + clrName), oneofProperty.SetMethod);

            clearDelegate = oneofAccessor.Clear;
        }

        public FieldDescriptor Descriptor => descriptor;

        public void Clear(IMessage message)
        {
            clearDelegate(message);
        }

        public object GetValue(IMessage message)
        {
            return getValueDelegate(message);
        }

        public void SetValue(IMessage message, object value)
        {
            setValueDelegate(message, value);
        }

        private static string GetDuFullTypeName(PropertyInfo oneofProperty, string clrName)
        {
            // TODO(bbus) Try to do this better.
            var fullName = oneofProperty.PropertyType.AssemblyQualifiedName;
            var nameI = fullName.IndexOf(',');
            return fullName.Substring(0, nameI) + "+" + clrName + fullName.Substring(nameI);
        }
    }
}
