using System;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Extra information provided by generated code when initializing a message or file descriptor.
    /// These are constructed as required, and are not long-lived. Hand-written code should
    /// never need to use this type.
    /// </summary>
    public sealed class GeneratedCodeInfo
    {
        private static readonly string[] EmptyNames = new string[0];
        private static readonly GeneratedCodeInfo[] EmptyCodeInfo = new GeneratedCodeInfo[0];

        /// <summary>
        /// Irrelevant for file descriptors; the CLR type for the message for message descriptors.
        /// </summary>
        public Type ClrType { get; private set; }

        /// <summary>
        /// Irrelevant for file descriptors; the CLR property names (in message descriptor field order)
        /// for fields in the message for message descriptors.
        /// </summary>
        public string[] PropertyNames { get; private set; }

        /// <summary>
        /// Irrelevant for file descriptors; the CLR property "base" names (in message descriptor oneof order)
        /// for oneofs in the message for message descriptors. It is expected that for a oneof name of "Foo",
        /// there will be a "FooCase" property and a "ClearFoo" method.
        /// </summary>
        public string[] OneofNames { get; private set; }

        /// <summary>
        /// The reflection information for types within this file/message descriptor. Elements may be null
        /// if there is no corresponding generated type, e.g. for map entry types.
        /// </summary>
        public GeneratedCodeInfo[] NestedTypes { get; private set; }

        /// <summary>
        /// The CLR types for enums within this file/message descriptor.
        /// </summary>
        public Type[] NestedEnums { get; private set; }

        /// <summary>
        /// Creates a GeneratedCodeInfo for a message descriptor, with nested types, nested enums, the CLR type, property names and oneof names.
        /// Each array parameter may be null, to indicate a lack of values.
        /// The parameter order is designed to make it feasible to format the generated code readably.
        /// </summary>
        public GeneratedCodeInfo(Type clrType, string[] propertyNames, string[] oneofNames, Type[] nestedEnums, GeneratedCodeInfo[] nestedTypes)
        {
            NestedTypes = nestedTypes ?? EmptyCodeInfo;
            NestedEnums = nestedEnums ?? ReflectionUtil.EmptyTypes;
            ClrType = clrType;
            PropertyNames = propertyNames ?? EmptyNames;
            OneofNames = oneofNames ?? EmptyNames;
        }

        /// <summary>
        /// Creates a GeneratedCodeInfo for a file descriptor, with only types and enums.
        /// </summary>
        public GeneratedCodeInfo(Type[] nestedEnums, GeneratedCodeInfo[] nestedTypes)
            : this(null, null, null, nestedEnums, nestedTypes)
        {
        }
    }
}