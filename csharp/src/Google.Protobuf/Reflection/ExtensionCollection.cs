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
using System.Collections.ObjectModel;
using System.Linq;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// A collection to simplify retrieving the descriptors of extensions in a descriptor for a message
    /// </summary>
    public class ExtensionCollection
    {
        private IDictionary<MessageDescriptor, IList<FieldDescriptor>> extensionsByTypeInDeclarationOrder;
        private IDictionary<MessageDescriptor, IList<FieldDescriptor>> extensionsByTypeInNumberOrder;

        internal ExtensionCollection(FileDescriptor file, Extension[] extensions)
        {
            UnorderedExtensions = DescriptorUtil.ConvertAndMakeReadOnly(
                file.Proto.Extension,
                (extension, i) => new FieldDescriptor(extension, file, null, i, null, extensions?[i]));
        }

        internal ExtensionCollection(MessageDescriptor message, Extension[] extensions)
        {
            UnorderedExtensions = DescriptorUtil.ConvertAndMakeReadOnly(
                message.Proto.Extension,
                (extension, i) => new FieldDescriptor(extension, message.File, message, i, null, extensions?[i]));
        }

        /// <summary>
        /// Returns a readonly list of all the extensions defined in this type in 
        /// the order they were defined in the source .proto file
        /// </summary>
        public IList<FieldDescriptor> UnorderedExtensions { get; }

        /// <summary>
        /// Returns a readonly list of all the extensions define in this type that extend 
        /// the provided descriptor type in the order they were defined in the source .proto file
        /// </summary>
        public IList<FieldDescriptor> GetExtensionsInDeclarationOrder(MessageDescriptor descriptor)
        {
            return extensionsByTypeInDeclarationOrder[descriptor];
        }

        /// <summary>
        /// Returns a readonly list of all the extensions define in this type that extend 
        /// the provided descriptor type in accending field order
        /// </summary>
        public IList<FieldDescriptor> GetExtensionsInNumberOrder(MessageDescriptor descriptor)
        {
            return extensionsByTypeInNumberOrder[descriptor];
        }

        internal void CrossLink()
        {
            Dictionary<MessageDescriptor, IList<FieldDescriptor>> declarationOrder = new Dictionary<MessageDescriptor, IList<FieldDescriptor>>();
            foreach (FieldDescriptor descriptor in UnorderedExtensions)
            {
                descriptor.CrossLink();

                IList<FieldDescriptor> _;
                if (!declarationOrder.TryGetValue(descriptor.ExtendeeType, out _))
                    declarationOrder.Add(descriptor.ExtendeeType, new List<FieldDescriptor>());

                declarationOrder[descriptor.ExtendeeType].Add(descriptor);
            }

            extensionsByTypeInDeclarationOrder = declarationOrder
                .ToDictionary(kvp => kvp.Key, kvp => (IList<FieldDescriptor>)new ReadOnlyCollection<FieldDescriptor>(kvp.Value));
            extensionsByTypeInNumberOrder = declarationOrder
                .ToDictionary(kvp => kvp.Key, kvp => (IList<FieldDescriptor>)new ReadOnlyCollection<FieldDescriptor>(kvp.Value.OrderBy(field => field.FieldNumber).ToArray()));
        }
    }
}
