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

using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  public interface IGeneratedExtensionLite {
    int Number { get; }
    object ContainingType { get; }
    IMessageLite MessageDefaultInstance { get; }
  }

  public class ExtensionDescriptorLite : IFieldDescriptorLite {
    /// <summary>
    /// Immutable mapping from field type to mapped type. Built using the attributes on
    /// FieldType values.
    /// </summary>
    public static readonly IDictionary<FieldType, MappedType> FieldTypeToMappedTypeMap = MapFieldTypes();

    private static IDictionary<FieldType, MappedType> MapFieldTypes() {
      var map = new Dictionary<FieldType, MappedType>();
      foreach (System.Reflection.FieldInfo field in typeof(FieldType).GetFields(System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.Public)) {
        FieldType fieldType = (FieldType)field.GetValue(null);
        FieldMappingAttribute mapping = (FieldMappingAttribute)field.GetCustomAttributes(typeof(FieldMappingAttribute), false)[0];
        map[fieldType] = mapping.MappedType;
      }
      return Dictionaries.AsReadOnly(map);
    }

    private readonly IEnumLiteMap enumTypeMap;
    private readonly int number;
    private readonly FieldType type;
    private readonly bool isRepeated;
    private readonly bool isPacked;
    private readonly MappedType mapType;
    private readonly object defaultValue;

    public ExtensionDescriptorLite(IEnumLiteMap enumTypeMap, int number, FieldType type, object defaultValue, bool isRepeated, bool isPacked) {
      this.enumTypeMap = enumTypeMap;
      this.number = number;
      this.type = type;
      this.mapType = FieldTypeToMappedTypeMap[type];
      this.isRepeated = isRepeated;
      this.isPacked = isPacked;
      this.defaultValue = defaultValue;
    }

    public bool IsRepeated {
      get { return isRepeated; }
    }

    public bool IsRequired {
      get { return false; }
    }

    public bool IsPacked {
      get { return isPacked; }
    }

    public bool IsExtension {
      get { return true; }
    }

#warning ToDo - Discover the meaning and purpose of this durring serialization and return the correct value
    public bool MessageSetWireFormat {
      get { return true; }
    }

    public int FieldNumber {
      get { return number; }
    }

    public IEnumLiteMap EnumType {
      get { return enumTypeMap; }
    }

    public FieldType FieldType {
      get { return type; }
    }

    public MappedType MappedType {
      get { return mapType; }
    }

    public object DefaultValue {
      get { return defaultValue; }
    }

    public int CompareTo(IFieldDescriptorLite other) {
      return FieldNumber.CompareTo(other.FieldNumber);
    }
  }
  
  public class GeneratedExtensionLite<TContainingType, TExtensionType> : IGeneratedExtensionLite
    where TContainingType : IMessageLite {

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
        ExtensionDescriptorLite descriptor) {
      this.containingTypeDefaultInstance = containingTypeDefaultInstance;
      this.messageDefaultInstance = messageDefaultInstance;
      this.defaultValue = defaultValue;
      this.descriptor = descriptor;
    }

    /** For use by generated code only. */
    public GeneratedExtensionLite(
        TContainingType containingTypeDefaultInstance,
        TExtensionType defaultValue,
        IMessageLite messageDefaultInstance,
        IEnumLiteMap enumTypeMap,
        int number,
        FieldType type)
      : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
          new ExtensionDescriptorLite(enumTypeMap, number, type, defaultValue,
            false /* isRepeated */, false /* isPacked */)) {
    }

    /** For use by generated code only. */
    public GeneratedExtensionLite(
      TContainingType containingTypeDefaultInstance,
      TExtensionType defaultValue,
      IMessageLite messageDefaultInstance,
      IEnumLiteMap enumTypeMap,
      int number,
      FieldType type,
      bool isPacked)
      : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
          new ExtensionDescriptorLite(enumTypeMap, number, type, defaultValue,
            true /* isRepeated */, isPacked)) {
    }

    /// <summary>
    /// Returns information about this extension
    /// </summary>
    public IFieldDescriptorLite Descriptor {
      get { return descriptor; }
    } 

    /// <summary>
    /// Returns the default value for this extension
    /// </summary>
    public TExtensionType DefaultValue {
      get { return defaultValue; }
    } 

    /// <summary>
    /// used for the extension registry
    /// </summary>
    object IGeneratedExtensionLite.ContainingType {
      get { return ContainingTypeDefaultInstance; }
    }
      /**
     * Default instance of the type being extended, used to identify that type.
     */
    public TContainingType ContainingTypeDefaultInstance {
      get {
        return containingTypeDefaultInstance;
      }
    }

    /** Get the field number. */
    public int Number {
      get {
        return descriptor.FieldNumber;
      }
    }
    /**
     * If the extension is an embedded message, this is the default instance of
     * that type.
     */
    public IMessageLite MessageDefaultInstance {
      get {
        return messageDefaultInstance;
      }
    }

    public object SingularFromReflectionType(object value) {
      return value;
    }
  }
}