#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Google.Protobuf.Collections;
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a "oneof" field collection in a message type: a set of
    /// fields of which at most one can be set in any particular message.
    /// </summary>
    public sealed class OneofDescriptor : DescriptorBase
    {
        private MessageDescriptor containingType;
        private IList<FieldDescriptor> fields;
        private readonly OneofAccessor accessor;

        internal OneofDescriptor(OneofDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index, string clrName)
            : base(file, file.ComputeFullName(parent, proto.Name), index, parent.Features.MergedWith(proto.Options?.Features))
        {
            this.Proto = proto;
            containingType = parent;
            file.DescriptorPool.AddSymbol(this);

            // It's useful to determine whether or not this is a synthetic oneof before cross-linking. That means
            // diving into the proto directly rather than using FieldDescriptor, but that's okay.
            var firstFieldInOneof = parent.Proto.Field.FirstOrDefault(fieldProto => fieldProto.HasOneofIndex && fieldProto.OneofIndex == index);
            IsSynthetic = firstFieldInOneof?.Proto3Optional ?? false;

            accessor = CreateAccessor(clrName);
        }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name => Proto.Name;

        // Visible for testing
        internal OneofDescriptorProto Proto { get; }

        /// <summary>
        /// Returns a clone of the underlying <see cref="OneofDescriptorProto"/> describing this oneof.
        /// Note that a copy is taken every time this method is called, so clients using it frequently
        /// (and not modifying it) may want to cache the returned value.
        /// </summary>
        /// <returns>A protobuf representation of this oneof descriptor.</returns>
        public OneofDescriptorProto ToProto() => Proto.Clone();

        /// <summary>
        /// Gets the message type containing this oneof.
        /// </summary>
        /// <value>
        /// The message type containing this oneof.
        /// </value>
        public MessageDescriptor ContainingType
        {
            get { return containingType; }
        }

        /// <summary>
        /// Gets the fields within this oneof, in declaration order.
        /// </summary>
        /// <value>
        /// The fields within this oneof, in declaration order.
        /// </value>
        public IList<FieldDescriptor> Fields { get { return fields; } }

        /// <summary>
        /// Returns <c>true</c> if this oneof is a synthetic oneof containing a proto3 optional field;
        /// <c>false</c> otherwise.
        /// </summary>
        public bool IsSynthetic { get; }

        /// <summary>
        /// Gets an accessor for reflective access to the values associated with the oneof
        /// in a particular message.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In descriptors for generated code, the value returned by this property will always be non-null.
        /// </para>
        /// <para>
        /// In dynamically loaded descriptors, the value returned by this property will current be null;
        /// if and when dynamic messages are supported, it will return a suitable accessor to work with
        /// them.
        /// </para>
        /// </remarks>
        /// <value>
        /// The accessor used for reflective access.
        /// </value>
        public OneofAccessor Accessor { get { return accessor; } }

        /// <summary>
        /// The (possibly empty) set of custom options for this oneof.
        /// </summary>
        [Obsolete("CustomOptions are obsolete. Use the GetOptions method.")]
        public CustomOptions CustomOptions => new CustomOptions(Proto.Options?._extensions?.ValuesByNumber);

        /// <summary>
        /// The <c>OneofOptions</c>, defined in <c>descriptor.proto</c>.
        /// If the options message is not present (i.e. there are no options), <c>null</c> is returned.
        /// Custom options can be retrieved as extensions of the returned message.
        /// NOTE: A defensive copy is created each time this property is retrieved.
        /// </summary>
        public OneofOptions GetOptions()
        {
            var clone = Proto.Options?.Clone();
            if (clone is null)
            {
                return null;
            }
            // Clients should be using feature accessor methods, not accessing features on the
            // options proto.
            clone.Features = null;
            return clone;
        }

        /// <summary>
        /// Gets a single value oneof option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public T GetOption<T>(Extension<OneofOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> ? (value as IDeepCloneable<T>).Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value oneof option for this descriptor
        /// </summary>
        [Obsolete("GetOption is obsolete. Use the GetOptions() method.")]
        public RepeatedField<T> GetOption<T>(RepeatedExtension<OneofOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }

        internal void CrossLink()
        {
            List<FieldDescriptor> fieldCollection = new List<FieldDescriptor>();
            foreach (var field in ContainingType.Fields.InDeclarationOrder())
            {
                if (field.ContainingOneof == this)
                {
                    fieldCollection.Add(field);
                }
            }
            fields = new ReadOnlyCollection<FieldDescriptor>(fieldCollection);
        }

        private OneofAccessor CreateAccessor(string clrName)
        {
            // We won't have a CLR name if this is from a dynamically-loaded FileDescriptor.
            // TODO: Support dynamic messages.
            if (clrName == null)
            {
                return null;
            }
            if (IsSynthetic)
            {
                return OneofAccessor.ForSyntheticOneof(this);
            }
            else
            {
                var caseProperty = containingType.ClrType.GetProperty(clrName + "Case");
                if (caseProperty == null)
                {
                    throw new DescriptorValidationException(this, $"Property {clrName}Case not found in {containingType.ClrType}");
                }
                if (!caseProperty.CanRead)
                {
                    throw new ArgumentException($"Cannot read from property {clrName}Case in {containingType.ClrType}");
                }
                var clearMethod = containingType.ClrType.GetMethod("Clear" + clrName);
                if (clearMethod == null)
                {
                    throw new DescriptorValidationException(this, $"Method Clear{clrName} not found in {containingType.ClrType}");
                }
                return OneofAccessor.ForRegularOneof(this, caseProperty, clearMethod);
            }
        }
    }
}
