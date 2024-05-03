#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Collections.Generic;
using System.Diagnostics;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Base class for nearly all descriptors, providing common functionality.
    /// </summary>
    [DebuggerDisplay("Type = {GetType().Name,nq}, FullName = {FullName}")]
    public abstract class DescriptorBase : IDescriptor
    {
        internal DescriptorBase(FileDescriptor file, string fullName, int index)
        {
            File = file;
            FullName = fullName;
            Index = index;
        }

        /// <value>
        /// The index of this descriptor within its parent descriptor.
        /// </value>
        /// <remarks>
        /// This returns the index of this descriptor within its parent, for
        /// this descriptor's type. (There can be duplicate values for different
        /// types, e.g. one enum type with index 0 and one message type with index 0.)
        /// </remarks>
        public int Index { get; }

        /// <summary>
        /// Returns the name of the entity (field, message etc) being described.
        /// </summary>
        public abstract string Name { get; }

        /// <summary>
        /// The fully qualified name of the descriptor's target.
        /// </summary>
        public string FullName { get; }

        /// <value>
        /// The file this descriptor was declared in.
        /// </value>
        public FileDescriptor File { get; }

        /// <summary>
        /// The declaration information about the descriptor, or null if no declaration information
        /// is available for this descriptor.
        /// </summary>
        /// <remarks>
        /// This information is typically only available for dynamically loaded descriptors,
        /// for example within a protoc plugin where the full descriptors, including source info,
        /// are passed to the code by protoc.
        /// </remarks>
        public DescriptorDeclaration Declaration => File.GetDeclaration(this);

        /// <summary>
        /// Retrieves the list of nested descriptors corresponding to the given field number, if any.
        /// If the field is unknown or not a nested descriptor list, return null to terminate the search.
        /// The default implementation returns null.
        /// </summary>
        internal virtual IReadOnlyList<DescriptorBase> GetNestedDescriptorListForField(int fieldNumber) => null;
    }
}