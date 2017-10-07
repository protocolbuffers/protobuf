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

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a "oneof" field collection in a message type: a set of
    /// fields of which at most one can be set in any particular message.
    /// </summary>
    public sealed class OneofDescriptor : DescriptorBase
    {
        private readonly OneofDescriptorProto proto;
        private MessageDescriptor containingType;
        private IList<FieldDescriptor> fields;
        private readonly OneofAccessor accessor;

        internal OneofDescriptor(OneofDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index, string clrName)
            : base(file, file.ComputeFullName(parent, proto.Name), index)
        {
            this.proto = proto;
            containingType = parent;

            file.DescriptorPool.AddSymbol(this);
            accessor = CreateAccessor(clrName);
        }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name { get { return proto.Name; } }

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
        /// Gets an accessor for reflective access to the values associated with the oneof
        /// in a particular message.
        /// </summary>
        /// <value>
        /// The accessor used for reflective access.
        /// </value>
        public OneofAccessor Accessor { get { return accessor; } }

        /// <summary>
        /// The (possibly empty) set of custom options for this oneof.
        /// </summary>
        public CustomOptions CustomOptions => proto.Options?.CustomOptions ?? CustomOptions.Empty;

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
            var caseProperty = containingType.ClrType.GetProperty(clrName + "Case");
            if (caseProperty == null)
            {
                throw new DescriptorValidationException(this, $"Property {clrName}Case not found in {containingType.ClrType}");
            }
            var clearMethod = containingType.ClrType.GetMethod("Clear" + clrName);
            if (clearMethod == null)
            {
                throw new DescriptorValidationException(this, $"Method Clear{clrName} not found in {containingType.ClrType}");
            }

            return new OneofAccessor(caseProperty, clearMethod, this);
        }
    }
}
