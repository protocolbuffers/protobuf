#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
 