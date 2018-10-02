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

using System.Collections.Generic;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Base class for nearly all descriptors, providing common functionality.
    /// </summary>
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