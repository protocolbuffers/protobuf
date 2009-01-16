using System;
using System.IO;

namespace Google.ProtocolBuffers.ProtoDump
{
  /// <summary>
  /// Small utility to load a binary message and dump it in text form
  /// </summary>
  class Program {
    static int Main(string[] args) {
      if (args.Length != 2) {
        Console.Error.WriteLine("Usage: ProtoDump <descriptor type name> <input data>");
        Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
        Console.Error.WriteLine("including assembly e.g. ProjectNamespace.Message,Company.Project");
        return 1;
      }
      IMessage defaultMessage;
      try {
        defaultMessage = MessageUtil.GetDefaultMessage(args[0]);
      } catch (ArgumentException e) {
        Console.Error.WriteLine(e.Message);
        return 1;
      }
      try {
        IBuilder builder = defaultMessage.WeakCreateBuilderForType();
        if (builder == null) {
          Console.Error.WriteLine("Unable to create builder");
          return 1;
        }
        byte[] inputData = File.ReadAllBytes(args[1]);
        builder.WeakMergeFrom(ByteString.CopyFrom(inputData));
        Console.WriteLine(TextFormat.PrintToString(builder.WeakBuild()));
        return 0;
      } catch (Exception e) {
        Console.Error.WriteLine("Error: {0}", e.Message);
        Console.Error.WriteLine();
        Console.Error.WriteLine("Detailed exception information: {0}", e);
        return 1;
      }
    }
  }
}