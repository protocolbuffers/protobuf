As part of the 3.10 release of Google.Protobuf, experimental proto2 support has been released. This document outlines the new changes brought about to include proto2 support. This does not break existing proto3 support and users may continue to use proto3 features without changing their current code. Again the generated code and public API associated with proto2 is experimental and subject to change in the future. APIs for proto2 may be added, removed, or adjusted as feedback is received.
Generated code for proto2 may also be modified by adding, removing, or adjusting APIs as feedback is received.

### Enabling proto2 features

For information about specific proto2 features, please read the [proto2 language guide](https://developers.google.com/protocol-buffers/docs/proto).

Much like other languages, proto2 features are used with proto2 files with the syntax declaration `syntax = "proto2";`. However, please note, proto3 is still the recommended version of protobuf and proto2 support is meant for legacy system interop and advanced uses.

# Generated code

### Messages

Messages in proto2 files are very similar to their proto3 counterparts. They expose the usual property for getting and setting,  but they also include properties and methods to handle field presence.

For `optional`/`required` field XYZ, a `HasXYZ` property is included for checking presence and a `ClearXYZ` method is included for clearing the value.

```proto
message Foo {
    optional Bar bar = 1;
    required Baz baz = 2;
}
```
```cs
var foo = new Foo();
Assert.IsNull(foo.Bar);
Assert.False(foo.HasBar);
foo.Bar = new Bar();
Assert.True(foo.HasBar);
foo.ClearBar();
```

### Messages with extension ranges

Messages which define extension ranges implement the `IExtendableMessage` interface as shown below.
See inline comments for more info.

```cs
public interface IExtendableMessage<T> : IMessage<T> where T : IExtendableMessage<T>
{
    // Gets the value of a single value extension. If the extension isn't present, this returns the default value.
    TValue GetExtension<TValue>(Extension<T, TValue> extension);
    // Gets the value of a repeated extension. If the extension hasn't been set, this returns null to prevent unnecessary allocations.
    RepeatedField<TValue> GetExtension<TValue>(RepeatedExtension<T, TValue> extension);
    // Gets the value of a repeated extension. This will initialize the value of the repeated field and will never return null.
    RepeatedField<TValue> GetOrInitializeExtension<TValue>(RepeatedExtension<T, TValue> extension);
    // Sets the value of the extension
    void SetExtension<TValue>(Extension<T, TValue> extension, TValue value);
    // Returns whether the extension is present in the message
    bool HasExtension<TValue>(Extension<T, TValue> extension);
    // Clears the value of the extension, removing it from the message
    void ClearExtension<TValue>(Extension<T, TValue> extension);
    // Clears the value of the repeated extension, removing it from the message. Calling GetExtension after this will always return null.
    void ClearExtension<TValue>(RepeatedExtension<T, TValue> extension);
}
```

### Extensions

Extensions are generated in static containers like reflection classes and type classes. 
For example for a file called `foo.proto` containing extensions in the file scope, a 
`FooExtensions` class is created containing the extensions defined in the file scope.
For easy access, this class can be used with `using static` to bring all extensions into scope.

```proto
option csharp_namespace = "FooBar";
extend Foo {
    optional Baz foo_ext = 124;
}
message Baz {
    extend Foo {
        repeated Baz repeated_foo_ext = 125;
    }
}
```
```cs
public static partial class FooExtensions {
    public static readonly Extension<Foo, Baz> FooExt = /* initialization */;
}

public partial class Baz {
    public partial static class Extensions {
        public static readonly RepeatedExtension<Foo, Baz> RepeatedFooExt = /* initialization */;
    }
}
```
```cs
using static FooBar.FooExtensions;
using static FooBar.Baz.Extensions;

var foo = new Foo();
foo.SetExtension(FooExt, new Baz());
foo.GetOrInitializeExtension(RepeatedFooExt).Add(new Baz());
```

# APIs

### Message initialization

Initialization refers to checking the status of required fields in a proto2 message. If a message is uninitialized, not all required fields are set in either the message itself or any of its submessages. In other languages, missing required fields throw errors depending on the merge method used. This could cause unforseen errors at runtime if the incorrect method is used. 
However, in this implementation, parsers and input streams don't check messages for initialization on their own and throw errors. Instead it's up to you to handle messages with missing required fields in whatever way you see fit.
Checking message initialization can be done manually via the `IsInitialized` extension method in `MessageExtensions`.

### Extension registries

Just like in Java, extension registries can be constructed to parse extensions when reading new messages
from input streams. The API is fairly similar to the Java API with some added bonuses with C# syntax sugars.

```proto
message Baz {
    extend Foo {
        optional Baz foo_ext = 124;
    }
}
```
```cs
var registry = new ExtensionRegistry() 
{ 
    Baz.Extensions.FooExt
};
var foo = Foo.Factory.WithExtensionRegistry(registry).ParseFrom(input);
Assert.True(foo.HasExtension(Bas.Extensions.FooExt));
var fooNoRegistry = Foo.Factory.ParseFrom(input);
Assert.False(foo.HasExtension(Bas.Extensions.FooExt));
```

### Custom options

Due to their limited use and lack of type safety, the original `CustomOptions` APIs are now deprecated. Using the new generated extension identifiers, you can access extensions safely through the GetOption APIs. Note that cloneable values such as 
repeated fields and messages will be deep cloned.

Example based on custom options usage example [here](https://github.com/protocolbuffers/protobuf/issues/5007#issuecomment-411604515).
```cs
foreach (var service in input.Services)
{
    Console.WriteLine($"  {service.Name}");
    foreach (var method in service.Methods)
    {
        var rule = method.GetOption(AnnotationsExtensions.Http);
        if (rule != null)
        {
            Console.WriteLine($"    {method.Name}: {rule}");
        }
        else
        {
            Console.WriteLine($"    {method.Name}: no HTTP binding");
        }
    }
}
```

### Reflection

Reflection APIs have been extended to enable accessing the new proto2 portions of the library and generated code.

 * FieldDescriptor.Extension
    * Gets the extension identifier behind an extension field, allowing it to be added to an ExtensionRegistry
 * FieldDescriptor.IsExtension
    * Returns whether a field is an extension of another type.
 * FieldDescriptor.ExtendeeType
    * Returns the extended type of an extension field
 * IFieldAccessor.HasValue
    * Returns whether a field's value is set. For proto3 fields, throws an InvalidOperationException.
 * FileDescriptor.Syntax
    * Gets the syntax of a file
 * FileDescriptor.Extensions
    * An immutable list of extensions defined in the file
 * MessageDescriptor.Extensions
    * An immutable list of extensions defined in the message

```cs
var extensions = Baz.Descriptor.Extensions.GetExtensionsInDeclarationOrder(Foo.Descriptor);
var registry = new ExtensionRegistry();
registry.AddRange(extensions.Select(f => f.Extension));

var baz = Foo.Descriptor.Parser.WithExtensionRegistry(registry).ParseFrom(input);
foreach (var field in extensions)
{
    if (field.Accessor.HasValue(baz))
    {
        Console.WriteLine($"{field.Name}: {field.Accessor.GetValue(baz)}");
    }
}
```
