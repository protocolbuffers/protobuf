#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf.Reflection
{
    internal sealed class ExtensionAccessor : IFieldAccessor
    {
        private readonly Extension extension;
        private readonly ReflectionUtil.IExtensionReflectionHelper helper;

        internal ExtensionAccessor(FieldDescriptor descriptor)
        {
            Descriptor = descriptor;
            extension = descriptor.Extension;
            helper = ReflectionUtil.CreateExtensionHelper(extension);
        }

        public FieldDescriptor Descriptor { get; }

        public void Clear(IMessage message)
        {
            helper.ClearExtension(message);
        }

        public bool HasValue(IMessage message)
        {
            return helper.HasExtension(message);
        }

        public object GetValue(IMessage message)
        {
            return helper.GetExtension(message);
        }

        public void SetValue(IMessage message, object value)
        {
            helper.SetExtension(message, value);
        }
    }
}
