#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    public interface IGeneratedExtensionLite
    {
        int Number { get; }
        object ContainingType { get; }
        IMessageLite MessageDefaultInstance { get; }
        IFieldDescriptorLite Descriptor { get; }
    }

    public class ExtensionDescriptorLite : IFieldDescriptorLite
    {
        private readonly string fullName;
        private readonly IEnumLiteMap enumTypeMap;
        private readonly int number;
        private readonly FieldType type;
        private readonly bool isRepeated;
        private readonly bool isPacked;
        private readonly MappedType mapType;
        private readonly object defaultValue;

        public ExtensionDescriptorLite(string fullName, IEnumLiteMap enumTypeMap, int number, FieldType type,
                                       object defaultValue, bool isRepeated, bool isPacked)
        {
            this.fullName = fullName;
            this.enumTypeMap = enumTypeMap;
            this.number = number;
            this.type = type;
            this.mapType = FieldMappingAttribute.MappedTypeFromFieldType(type);
            this.isRepeated = isRepeated;
            this.isPacked = isPacked;
            this.defaultValue = defaultValue;
        }

        public string Name
        {
            get
            {
                string name = fullName;
                int offset = name.LastIndexOf('.');
                if (offset >= 0)
                {
                    name = name.Substring(offset);
                }
                return name;
            }
        }

        public string FullName
        {
            get { return fullName; }
        }

        public bool IsRepeated
        {
            get { return isRepeated; }
        }

        public bool IsRequired
        {
            get { return false; }
        }

        public bool IsPacked
        {
            get { return isPacked; }
        }

        public bool IsExtension
        {
            get { return true; }
        }

        /// <summary>
        /// This is not supported and assertions are made to ensure this does not exist on extensions of Lite types
        /// </summary>
        public bool MessageSetWireFormat
        {
            get { return false; }
        }

        public int FieldNumber
        {
            get { return number; }
        }

        public IEnumLiteMap EnumType
        {
            get { return enumTypeMap; }
        }

        public FieldType FieldType
        {
            get { return type; }
        }

        public MappedType MappedType
        {
            get { return mapType; }
        }

        public object DefaultValue
        {
            get { return defaultValue; }
        }

        public int CompareTo(IFieldDescriptorLite other)
        {
            return FieldNumber.CompareTo(other.FieldNumber);
        }
    }

    public class GeneratedRepeatExtensionLite<TContainingType, TExtensionType> :
        GeneratedExtensionLite<TContainingType, IList<TExtensionType>>
        where TContainingType : IMessageLite
    {
        public GeneratedRepeatExtensionLite(string fullName, TContainingType containingTypeDefaultInstance,
                                            IMessageLite messageDefaultInstance, IEnumLiteMap enumTypeMap, int number,
                                            FieldType type, bool isPacked) :
                                                base(
                                                fullName, containingTypeDefaultInstance, new List<TExtensionType>(),
                                                messageDefaultInstance, enumTypeMap, number, type, isPacked)
        {
        }

        public override object ToReflectionType(object value)
        {
            IList<object> result = new List<object>();
            foreach (object element in (IEnumerable) value)
            {
                result.Add(SingularToReflectionType(element));
            }
            return result;
        }

        public override object FromReflectionType(object value)
        {
            // Must convert the whole list.
            List<TExtensionType> result = new List<TExtensionType>();
            foreach (object element in (IEnumerable) value)
            {
                result.Add((TExtensionType) SingularFromReflectionType(element));
            }
            return result;
        }
    }

    public class GeneratedExtensionLite<TContainingType, TExtensionType> : IGeneratedExtensionLite
        where TContainingType : IMessageLite
    {
        private readonly TContainingType containingTypeDefaultInstance;
        private readonly TExtensionType defaultValue;
        private readonly IMessageLite messageDefaultInstance;
        private readonly ExtensionDescriptorLite descriptor;

        // We can't always initialize a GeneratedExtension when we first construct
        // it due to initialization order difficulties (namely, the default
        // instances may not have been constructed yet).  So, we construct an
        // uninitialized GeneratedExtension once, then call internalInit() on it
        // later.  Generated code will always call internalInit() on all extensions
        // as part of the static initialization code, and internalInit() throws an
        // exception if called more than once, so this method is useless to users.
        protected GeneratedExtensionLite(
            TContainingType containingTypeDefaultInstance,
            TExtensionType defaultValue,
            IMessageLite messageDefaultInstance,
            ExtensionDescriptorLite descriptor)
        {
            this.containingTypeDefaultInstance = containingTypeDefaultInstance;
            this.messageDefaultInstance = messageDefaultInstance;
            this.defaultValue = defaultValue;
            this.descriptor = descriptor;
        }

        /** For use by generated code only. */

        public GeneratedExtensionLite(
            string fullName,
            TContainingType containingTypeDefaultInstance,
            TExtensionType defaultValue,
            IMessageLite messageDefaultInstance,
            IEnumLiteMap enumTypeMap,
            int number,
            FieldType type)
            : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
                   new ExtensionDescriptorLite(fullName, enumTypeMap, number, type, defaultValue,
                                               false /* isRepeated */, false /* isPacked */))
        {
        }

        private static readonly IList<object> Empty = new object[0];
        /** Repeating fields: For use by generated code only. */

        protected GeneratedExtensionLite(
            string fullName,
            TContainingType containingTypeDefaultInstance,
            TExtensionType defaultValue,
            IMessageLite messageDefaultInstance,
            IEnumLiteMap enumTypeMap,
            int number,
            FieldType type,
            bool isPacked)
            : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
                   new ExtensionDescriptorLite(fullName, enumTypeMap, number, type, Empty,
                                               true /* isRepeated */, isPacked))
        {
        }

        /// <summary>
        /// Returns information about this extension
        /// </summary>
        public IFieldDescriptorLite Descriptor
        {
            get { return descriptor; }
        }

        /// <summary>
        /// Returns the default value for this extension
        /// </summary>
        public TExtensionType DefaultValue
        {
            get { return defaultValue; }
        }

        /// <summary>
        /// used for the extension registry
        /// </summary>
        object IGeneratedExtensionLite.ContainingType
        {
            get { return ContainingTypeDefaultInstance; }
        }

        /**
     * Default instance of the type being extended, used to identify that type.
     */

        public TContainingType ContainingTypeDefaultInstance
        {
            get { return containingTypeDefaultInstance; }
        }

        /** Get the field number. */

        public int Number
        {
            get { return descriptor.FieldNumber; }
        }

        /**
     * If the extension is an embedded message, this is the default instance of
     * that type.
     */

        public IMessageLite MessageDefaultInstance
        {
            get { return messageDefaultInstance; }
        }

        /// <summary>
        /// Converts from the type used by the native accessors to the type
        /// used by reflection accessors. For example, the reflection accessors
        /// for enums use EnumValueDescriptors but the native accessors use
        /// the generated enum type.
        /// </summary>
        public virtual object ToReflectionType(object value)
        {
            return SingularToReflectionType(value);
        }

        /// <summary>
        /// Like ToReflectionType(object) but for a single element.
        /// </summary>
        public object SingularToReflectionType(object value)
        {
            return descriptor.MappedType == MappedType.Enum
                       ? descriptor.EnumType.FindValueByNumber((int) value)
                       : value;
        }

        public virtual object FromReflectionType(object value)
        {
            return SingularFromReflectionType(value);
        }

        public object SingularFromReflectionType(object value)
        {
            switch (Descriptor.MappedType)
            {
                case MappedType.Message:
                    if (value is TExtensionType)
                    {
                        return value;
                    }
                    else
                    {
                        // It seems the copy of the embedded message stored inside the
                        // extended message is not of the exact type the user was
                        // expecting.  This can happen if a user defines a
                        // GeneratedExtension manually and gives it a different type.
                        // This should not happen in normal use.  But, to be nice, we'll
                        // copy the message to whatever type the caller was expecting.
                        return MessageDefaultInstance.WeakCreateBuilderForType()
                            .WeakMergeFrom((IMessageLite) value).WeakBuild();
                    }
                case MappedType.Enum:
                    // Just return a boxed int - that can be unboxed to the enum
                    IEnumLite enumValue = (IEnumLite) value;
                    return enumValue.Number;
                default:
                    return value;
            }
        }
    }
}