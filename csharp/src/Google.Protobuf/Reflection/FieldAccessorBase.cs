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
    /// Base class for field accessors.
    /// </summary>
    internal abstract class FieldAccessorBase : IFieldAccessor
    {
        private readonly Func<IMessage, object> getValueDelegate;

        internal FieldAccessorBase(PropertyInfo property, FieldDescriptor descriptor)
        {
            Descriptor = descriptor;
            getValueDelegate = ReflectionUtil.CreateFuncIMessageObject(property.GetGetMethod());
        }

        public FieldDescriptor Descriptor { get; }

        public object GetValue(IMessage message)
        {
            return getValueDelegate(message);
        }

        public abstract bool HasValue(IMessage message);
        public abstract void Clear(IMessage message);
        public abstract void SetValue(IMessage message, object value);
    }
}
