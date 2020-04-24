namespace Google.Protobuf.Reflection
{
    public sealed partial class FileOptions 
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class MessageOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class FieldOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class EnumOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class EnumValueOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class ServiceOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class MethodOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }

    public sealed partial class OneofOptions
    {
        internal CustomOptions CreateCustomOptions() => new CustomOptions(_extensions?.ValuesByNumber, _unknownFields);
    }
}