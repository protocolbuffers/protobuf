using System;
using System.Collections.Generic;
using System.Text;
using System.Collections;
using System.IO;
using System.Reflection;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Helper class to create MessageStreamIterators without explicitly specifying
  /// the builder type. The concrete builder type is determined by reflection, based
  /// on the message type. The reflection step looks for a <c>CreateBuilder</c> method
  /// in the message type which is public and static, and uses the return type of that
  /// method as the builder type. This will work for all generated messages, whether
  /// optimised for size or speed. It won't work for dynamic messages.
  /// 
  /// TODO(jonskeet): This also won't work for non-public protos :(
  /// Pass in delegate to create a builder?
  /// </summary>
  public static class MessageStreamIterator {

    public static IEnumerable<TMessage> FromFile<TMessage>(string file) 
        where TMessage : IMessage<TMessage> {
      return FromStreamProvider<TMessage>(() => File.OpenRead(file));
    }

    public static IEnumerable<TMessage> FromStreamProvider<TMessage>(StreamProvider streamProvider)
        where TMessage : IMessage<TMessage> {
      MethodInfo createBuilderMethod = typeof(TMessage).GetMethod("CreateBuilder", Type.EmptyTypes);
      if (createBuilderMethod == null) {
        throw new ArgumentException("Message type " + typeof(TMessage).FullName + " has no CreateBuilder method.");
      }
      if (createBuilderMethod.ReturnType == typeof(void)) {
        throw new ArgumentException("CreateBuilder method in " + typeof(TMessage).FullName + " has void return type");
      }
      Type builderType = createBuilderMethod.ReturnType;
      if (builderType.GetConstructor(Type.EmptyTypes) == null) {
        throw new ArgumentException("Builder type " + builderType.FullName + " has no public parameterless constructor.");
      }
      Type messageInterface = typeof(IMessage<,>).MakeGenericType(typeof(TMessage), builderType);
      Type builderInterface = typeof(IBuilder<,>).MakeGenericType(typeof(TMessage), builderType);
      if (Array.IndexOf(typeof (TMessage).GetInterfaces(), messageInterface) == -1) {
        throw new ArgumentException("Message type " + typeof(TMessage) + " doesn't implement " + messageInterface.FullName);
      }
      if (Array.IndexOf(builderType.GetInterfaces(), builderInterface) == -1) {
        throw new ArgumentException("Builder type " + typeof(TMessage) + " doesn't implement " + builderInterface.FullName);
      }
      Type iteratorType = typeof(MessageStreamIterator<,>).MakeGenericType(typeof(TMessage), builderType);
      MethodInfo factoryMethod = iteratorType.GetMethod("FromStreamProvider", new Type[] { typeof(StreamProvider) });
      return (IEnumerable<TMessage>) factoryMethod.Invoke(null, new object[] { streamProvider });
    }
  }

  /// <summary>
  /// Iterates over data created using a <see cref="MessageStreamWriter{T}" />.
  /// Unlike MessageStreamWriter, this class is not usually constructed directly with
  /// a stream; instead it is provided with a way of opening a stream when iteration
  /// is started. The stream is closed when the iteration is completed or the enumerator
  /// is disposed. (This occurs naturally when using <c>foreach</c>.)
  /// This type is generic in both the message type and the builder type; if only the
  /// iteration is required (i.e. just <see cref="IEnumerable{T}" />) then the static
  /// generic methods in the nongeneric class are more appropriate.
  /// </summary>
  public sealed class MessageStreamIterator<TMessage, TBuilder> : IEnumerable<TMessage> 
      where TMessage : IMessage<TMessage, TBuilder>
      where TBuilder : IBuilder<TMessage, TBuilder>, new() {

    private readonly StreamProvider streamProvider;
    private readonly ExtensionRegistry extensionRegistry;
    private static readonly uint ExpectedTag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);

    private MessageStreamIterator(StreamProvider streamProvider, ExtensionRegistry extensionRegistry) {
      this.streamProvider = streamProvider;
      this.extensionRegistry = extensionRegistry;
    }

    /// <summary>
    /// Creates an instance which opens the specified file when it begins
    /// iterating. No extension registry is used when reading messages.
    /// </summary>
    public static MessageStreamIterator<TMessage, TBuilder> FromFile(string file) {
      return new MessageStreamIterator<TMessage, TBuilder>(() => File.OpenRead(file), ExtensionRegistry.Empty);
    }

    /// <summary>
    /// Creates an instance which calls the given delegate when it begins
    /// iterating. No extension registry is used when reading messages.
    /// </summary>
    public static MessageStreamIterator<TMessage, TBuilder> FromStreamProvider(StreamProvider streamProvider) {
      return new MessageStreamIterator<TMessage, TBuilder>(streamProvider, ExtensionRegistry.Empty);
    }

    /// <summary>
    /// Creates a new instance which uses the same stream provider as this one,
    /// but the specified extension registry.
    /// </summary>
    public MessageStreamIterator<TMessage, TBuilder> WithExtensionRegistry(ExtensionRegistry newRegistry) {
      return new MessageStreamIterator<TMessage, TBuilder>(streamProvider, newRegistry);
    }

    public IEnumerator<TMessage> GetEnumerator() {
      using (Stream stream = streamProvider()) {
        CodedInputStream input = CodedInputStream.CreateInstance(stream);
        uint tag;
        while ((tag = input.ReadTag()) != 0) {
          if (tag != ExpectedTag) {
            throw InvalidProtocolBufferException.InvalidMessageStreamTag();
          }
          TBuilder builder = new TBuilder();
          input.ReadMessage(builder, extensionRegistry);
          yield return builder.Build();
        }
      }
    }

    /// <summary>
    /// Explicit implementation of nongeneric IEnumerable interface.
    /// </summary>
    IEnumerator IEnumerable.GetEnumerator() {
      return GetEnumerator();
    }
  }
}
