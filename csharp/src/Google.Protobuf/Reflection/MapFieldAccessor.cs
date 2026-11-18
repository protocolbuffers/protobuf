#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Reflection;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Accessor for map fields.
    /// </summary>
    internal sealed class MapFieldAccessor : FieldAccessorBase
    {
        internal MapFieldAccessor(PropertyInfo property, FieldDescriptor descriptor) : base(property, descriptor)
        {
        }

        public override void Clear(IMessage message)
        {
            IDictionary list = (IDictionary) GetValue(message);
            list.Clear();
        }

        public override bool HasValue(IMessage message)
        {
            throw new InvalidOperationException("HasValue is not implemented for map fields");
        }

        public override void SetValue(IMessage message, object value)
        {
            throw new InvalidOperationException("SetValue is not implemented for map fields");
        }
    }
}
