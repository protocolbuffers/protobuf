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
using System.Collections.Generic;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Descriptor for an enum type in a .proto file.
    /// </summary>
    public sealed class EnumDescriptor : DescriptorBase
    {
        internal EnumDescriptor(EnumDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index, Type clrType)
            : base(file, file.ComputeFullName(parent, proto.Name), index)
        {
            Proto = proto;
            ClrType = clrType;
            ContainingType = parent;

            if (proto.Value.Count == 0)
            {
                // We cannot allow enums with no values because this would mean there
                // would be no valid default value for fields of this type.
                throw new DescriptorValidationException(this, "Enums must contain at least one value.");
            }

            Values = DescriptorUtil.ConvertAndMakeReadOnly(proto.Value,
                                                           (value, i) => new EnumValueDescriptor(value, file, this, i));

            File.DescriptorPool.AddSymbol(this);
        }

        internal EnumDescriptorProto Proto { get; }

        /// <summary>
        /// Returns a clone of the underlying <see cref="EnumDescriptorProto"/> describing this enum.
        /// Note that a copy is taken every time this method is called, so clients using it frequently
        /// (and not modifying it) may want to cache the returned value.
        /// </summary>
        /// <returns>A protobuf representation of this enum descriptor.</returns>
        public EnumDescriptorProto ToProto() => Proto.Clone();

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name => Proto.Name;

        internal override IReadOnlyList<DescriptorBase> GetNestedDescriptorListForField(int fieldNumber) =>
            fieldNumber switch
            {
                EnumDescriptorProto.ValueFieldNumber => (IReadOnlyList<DescriptorBase>)Values,
                _ => null,
            };

        /// <summary>
        /// The CLR type for this enum. For generated code, this will be a CLR enum type.
        /// </summary>
        public Type ClrType { get; }

        /// <value>
        /// If this is a nested type, get the outer descriptor, otherwise null.
        /// </value>
        public MessageDescriptor ContainingType { get; }

        /// <value>
        /// An unmodifiable list of defined value descriptors for this enum.
        /// </value>
        public IList<EnumValueDescriptor> Values { get; }

        /// <summary>
        /// Finds an enum value by number. If multiple enum values have the
        /// same number, this returns the first defined value with that number.
        /// If there is no value for the given number, this returns <c>null</c>.
        /// </summary>
        public EnumValueDescriptor FindValueByNumber(int number)
        {
            return File.DescriptorPool.FindEnumValueByNumber(this, number);
        }

        /// <summary>
        /// Finds an enum value by name.
        /// </summary>
        /// <param name="name">The unqualified name of the value (e.g. "FOO").</param>
        /// <returns>The value's descriptor, or null if not found.</returns>
        public EnumValueDescriptor FindValueByName(string name)
        {
            return File.DescriptorPool.FindSymbol<EnumValueDescriptor>(FullName + "." + name);
        }

        /// <summary>
        /// The (possibly empty) set of custom options for this enum.
        /// </summary>
        [Obsolete("CustomOptions are obsolete. Use the GetOptions() method.")]
        public CustomOptions CustomOptions => new CustomOptions(Proto.Options?._extensions?.ValuesByNumber);

        /// <summary>
        /// The <c>EnumOptions</c>, defined in <c>descriptor.proto</c>.
        /// If the options message is not present (i.e. there are no options), <c>null</c> is returned.
        /// Custom options can be retrieved as extensions of the returned message.
        /// NOTE: A defensive copy is created each time this property is retrieved.
        /// </summary>
        public EnumOptions GetOptions() => Proto.Options?.Clone();

        /// <summary>
        /// Gets a single value enum option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public T GetOption<T>(Extension<EnumOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> ? (value as IDeepCloneable<T>).Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value enum option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public RepeatedField<T> GetOption<T>(RepeatedExtension<EnumOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }
    }
}