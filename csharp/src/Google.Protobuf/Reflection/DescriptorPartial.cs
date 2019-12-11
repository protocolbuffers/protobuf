namespace Google.Protobuf.Reflection
{
    public sealed partial class FileOptions 
    {
        internal ExtensionSet<FileOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class MessageOptions
    {
        internal ExtensionSet<MessageOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class FieldOptions
    {
        internal ExtensionSet<FieldOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class EnumOptions
    {
        internal ExtensionSet<EnumOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class EnumValueOptions
    {
        internal ExtensionSet<EnumValueOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class ServiceOptions
    {
        internal ExtensionSet<ServiceOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class MethodOptions
    {
        internal ExtensionSet<MethodOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }

    public sealed partial class OneofOptions
    {
        internal ExtensionSet<OneofOptions> Extensions => _extensions;
        internal UnknownFieldSet UnknownFields => _unknownFields;
    }
}