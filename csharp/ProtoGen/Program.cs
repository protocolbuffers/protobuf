using System;
using System.Collections.Generic;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Entry point for the Protocol Buffers generator.
  /// </summary>
  class Program {
    static int Main(string[] args) {
      try {

        GeneratorOptions options = ParseCommandLineArguments(args);

        IList<string> validationFailures;
        if (!options.TryValidate(out validationFailures)) {
          // We've already got the message-building logic in the exception...
          InvalidOptionsException exception = new InvalidOptionsException(validationFailures);
          Console.WriteLine(exception.Message);
          return 1;
        }

        Generator generator = Generator.CreateGenerator(options);
        generator.Generate();


        return 0;
      } catch (Exception e) {
        Console.Error.WriteLine("Caught unhandled exception: {0}", e);
        return 1;
      }
    }

    private static GeneratorOptions ParseCommandLineArguments(string[] args) {
      GeneratorOptions options = new GeneratorOptions();
      string baseDir = "c:\\Users\\Jon\\Documents\\Visual Studio 2008\\Projects\\ProtocolBuffers";
      options.OutputDirectory = baseDir + "\\tmp";
      options.InputFiles = new[] { baseDir + "\\protos\\nwind.protobin" };
      return options;
    }
  }
}