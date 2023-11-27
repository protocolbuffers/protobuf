#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;
using System;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Descriptor for a single enum value within an enum in a .proto file.
    /// </summary>
    public sealed class EnumValueDescriptor : DescriptorBase
    {
        internal EnumValueDescriptor(EnumValueDescriptorProto proto, FileDescriptor file,
                                     EnumDescriptor parent, int index)
            : base(file, parent.FullName + "." + proto.Name, index)
        {
            Proto = proto;
            EnumDescriptor = parent;
            file.DescriptorPool.AddSymbol(this);
            file.DescriptorPool.AddEnumValueByNumber(this);
        }

        internal EnumValueDescriptorProto Proto { get; }

        /// <summary>
        /// Returns a clone of the underlying <see cref="EnumValueDescriptorProto"/> describing this enum value.
        /// Note that a copy is taken every time this method is called, so clients using it frequently
        /// (and not modifying it) may want to cache the returned value.
        /// </summary>
        /// <returns>A protobuf representation of this enum value descriptor.</returns>
        public EnumValueDescriptorProto ToProto() => Proto.Clone();

        /// <summary>
        /// Returns the name of the enum value described by this object.
        /// </summary>
        public override string Name => Proto.Name;

        /// <summary>
        /// Returns the number associated with this enum value.
        /// </summary>
        public int Number => Proto.Number;

        /// <summary>
        /// Returns the enum descriptor that this value is part of.
        /// </summary>
        public EnumDescriptor EnumDescriptor { get; }

        /// <summary>
        /// The (possibly empty) set of custom options for this enum value.
        /// </summary>
        [Obsolete("CustomOptions are obsolete. Use the GetOptions() method.")]
        public CustomOptions CustomOptions => new CustomOptions(Proto.Options?._extensions?.ValuesByNumber);

        /// <summary>
        /// The <c>EnumValueOptions</c>, defined in <c>descriptor.proto</c>.
        /// If the options message is not present (i.e. there are no options), <c>null</c> is returned.
        /// Custom options can be retrieved as extensions of the returned message.
        /// NOTE: A defensive copy is created each time this property is retrieved.
        /// </summary>
        public EnumValueOptions GetOptions() => Proto.Options?.Clone();

        /// <summary>
        /// Gets a single value enum value option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public T GetOption<T>(Extension<EnumValueOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> ? (value as IDeepCloneable<T>).Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value enum value option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public RepeatedField<T> GetOption<T>(RepeatedExtension<EnumValueOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }
    }
}
 