using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using System.Reflection;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Iterates over data created using a <see cref="MessageStreamWriter{T}" />.
  /// Unlike MessageStreamWriter, this class is not usually constructed directly with
  /// a stream; instead it is provided with a way of opening a stream when iteration
  /// is started. The stream is closed when the iteration is completed or the enumerator
  /// is disposed. (This occurs naturally when using <c>foreach</c>.)
  /// </summary>
  public class MessageStreamIterator<TMessage> : IEnumerable<TMessage>
      where TMessage : IMessage<TMessage> {

    private readonly StreamProvider streamProvider;
    private readonly ExtensionRegistry extensionRegistry;

    /// <summary>
    /// Delegate created via reflection trickery (once per type) to create a builder
    /// and read a message from a CodedInputStream with it. Note that unlike in Java,
    /// there's one static field per constructed type.
    /// </summary>
    private static readonly Func<CodedInputStream, ExtensionRegistry, TMessage> messageReader = BuildMessageReader();

    /// <summary>
    /// Any exception (within reason) thrown within messageReader is caught and rethrown in the constructor.
    /// This makes life a lot simpler for the caller.
    /// </summary>
    private static Exception typeInitializationException;

    /// <summary>
    /// Creates the delegate later used to read messages. This is only called once per type, but to
    /// avoid exceptions occurring at confusing times, if this fails it will set typeInitializationException
    /// to the appropriate error and return null.
    /// </summary>
    private static Func<CodedInputStream, ExtensionRegistry, TMessage> BuildMessageReader() {
      try {
        Type builderType = FindBuilderType();

        // Yes, it's redundant to find this again, but it's only the once...
        MethodInfo createBuilderMethod = typeof(TMessage).GetMethod("CreateBuilder", Type.EmptyTypes);
        Delegate builderBuilder = Delegate.CreateDelegate(
            typeof(Func<>).MakeGenericType(builderType), null, createBuilderMethod);

        MethodInfo buildMethod = typeof(MessageStreamIterator<TMessage>)
            .GetMethod("BuildImpl", BindingFlags.Static | BindingFlags.NonPublic)
            .MakeGenericMethod(typeof(TMessage), builderType);

        return (Func<CodedInputStream, ExtensionRegistry, TMessage>)Delegate.CreateDelegate(
          typeof(Func<CodedInputStream, ExtensionRegistry, TMessage>), builderBuilder, buildMethod);
      } catch (ArgumentException e) {
        typeInitializationException = e;
      } catch (InvalidOperationException e) {
        typeInitializationException = e;
      } catch (InvalidCastException e) {
        // Can't see why this would happen, but best to know about it.
        typeInitializationException = e;
      }
      return null;
    }

    /// <summary>
    /// Works out the builder type for TMessage, or throws an ArgumentException to explain why it can't.
    /// </summary>
    private static Type FindBuilderType() {
      MethodInfo createBuilderMethod = typeof(TMessage).GetMethod("CreateBuilder", Type.EmptyTypes);
      if (createBuilderMethod == null) {
        throw new ArgumentException("Message type " + typeof(TMessage).FullName + " has no CreateBuilder method.");
      }
      if (createBuilderMethod.ReturnType == typeof(void)) {
        throw new ArgumentException("CreateBuilder method in " + typeof(TMessage).FullName + " has void return type");
      }
      Type builderType = createBuilderMethod.ReturnType;
      Type messageInterface = typeof(IMessage<,>).MakeGenericType(typeof(TMessage), builderType);
      Type builderInterface = typeof(IBuilder<,>).MakeGenericType(typeof(TMessage), builderType);
      if (Array.IndexOf(typeof(TMessage).GetInterfaces(), messageInterface) == -1) {
        throw new ArgumentException("Message type " + typeof(TMessage) + " doesn't implement " + messageInterface.FullName);
      }
      if (Array.IndexOf(builderType.GetInterfaces(), builderInterface) == -1) {
        throw new ArgumentException("Builder type " + typeof(TMessage) + " doesn't implement " + builderInterface.FullName);
      }
      return builderType;
    }

// This is only ever fetched by reflection, so the compiler may
// complain that it's unused
#pragma warning disable 0414
    /// <summary>
    /// Method we'll use to build messageReader, with the first parameter fixed to TMessage.CreateBuilder. Note that we
    /// have to introduce another type parameter (TMessage2) as we can't constrain TMessage for just a single method
    /// (and we can't do it at the type level because we don't know TBuilder). However, by constraining TMessage2
    /// to not only implement IMessage appropriately but also to derive from TMessage2, we can avoid doing a cast
    /// for every message; the implicit reference conversion will be fine. In practice, TMessage2 and TMessage will
    /// be the same type when we construct the generic method by reflection.
    /// </summary>
    private static TMessage BuildImpl<TMessage2, TBuilder>(Func<TBuilder> builderBuilder, CodedInputStream input, ExtensionRegistry registry) 
        where TBuilder : IBuilder<TMessage2, TBuilder>
        where TMessage2 : TMessage, IMessage<TMessage2, TBuilder> {
      TBuilder builder = builderBuilder();
      input.ReadMessage(builder, registry);
      return builder.Build();
        }
#pragma warning restore 0414

    private static readonly uint ExpectedTag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);

    private MessageStreamIterator(StreamProvider streamProvider, ExtensionRegistry extensionRegistry) {
      if (messageReader == null) {
        throw typeInitializationException;
      }
      this.streamProvider = streamProvider;
      this.extensionRegistry = extensionRegistry;
    }

    /// <summary>
    /// Creates a new instance which uses the same stream provider as this one,
    /// but the specified extension registry.
    /// </summary>
    public MessageStreamIterator<TMessage> WithExtensionRegistry(ExtensionRegistry newRegistry) {
      return new MessageStreamIterator<TMessage>(streamProvider, newRegistry);
    }

    public static MessageStreamIterator<TMessage> FromFile(string file) {
      return new MessageStreamIterator<TMessage>(() => File.OpenRead(file), ExtensionRegistry.Empty);
    }

    public static MessageStreamIterator<TMessage> FromStreamProvider(StreamProvider streamProvider) {
      return new MessageStreamIterator<TMessage>(streamProvider, ExtensionRegistry.Empty);
    }

    public IEnumerator<TMessage> GetEnumerator() {
      using (Stream stream = streamProvider()) {
        CodedInputStream input = CodedInputStream.CreateInstance(stream);
        uint tag;
        while ((tag = input.ReadTag()) != 0) {
          if (tag != ExpectedTag) {
            throw InvalidProtocolBufferException.InvalidMessageStreamTag();
          }
          yield return messageReader(input, extensionRegistry);
        }
      }
    }

    IEnumerator IEnumerable.GetEnumerator() {
      return GetEnumerator();
    }
  }
}
