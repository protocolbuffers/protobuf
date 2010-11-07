using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  public interface IGeneratedExtensionLite {
    int Number { get; }
    object ContainingType { get; }
    IMessageLite MessageDefaultInstance { get; }
  }

  public class ExtensionDescriptorLite {
    private readonly EnumLiteMap enumTypeMap;
    private readonly int number;
    private readonly FieldType type;
    private readonly bool isRepeated;
    private readonly bool isPacked;

    public ExtensionDescriptorLite(EnumLiteMap enumTypeMap, int number, FieldType type, bool isRepeated, bool isPacked) {
      this.enumTypeMap = enumTypeMap;
      this.number = number;
      this.type = type;
      this.isRepeated = isRepeated;
      this.isPacked = isPacked;
    }

    public int Number {
      get { return number; }
    }
  }
  
  public class EnumLiteMap { }

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
      EnumLiteMap enumTypeMap,
        int number,
        FieldType type)
      : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
          new ExtensionDescriptorLite(enumTypeMap, number, type,
            false /* isRepeated */, false /* isPacked */)) {
    }

    /** For use by generated code only. */
    public GeneratedExtensionLite(
      TContainingType containingTypeDefaultInstance,
      TExtensionType defaultValue,
      IMessageLite messageDefaultInstance,
      EnumLiteMap enumTypeMap,
      int number,
      FieldType type,
      bool isPacked)
      : this(containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
          new ExtensionDescriptorLite(enumTypeMap, number, type,
            true /* isRepeated */, isPacked)) {
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
        return descriptor.Number;
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
  }
}