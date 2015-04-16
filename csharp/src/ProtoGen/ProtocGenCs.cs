using Google.ProtocolBuffers.Compiler.PluginProto;
using Google.ProtocolBuffers.DescriptorProtos;
using System;
using System.Collections.Generic;

// Usage example:
//   protoc.exe
//     --plugin=path\to\protoc-gen-cs.exe
//     --cs_out="-generated_code_attributes=true umbrella_namespace=TutorialProto :."
//     --proto_path=.\protos\
//     protos\tutorial\addressbook.proto

namespace Google.ProtocolBuffers.ProtoGen
{
    public static class ProtocGenCs
    {
        internal static void Run(CodeGeneratorRequest request, CodeGeneratorResponse.Builder response)
        {
            var arguments = new List<string>();
            foreach (var arg in request.Parameter.Split(' '))
            {
                var timmedArg = (arg ?? "").Trim();
                if (!string.IsNullOrEmpty(timmedArg))
                {
                    arguments.Add(timmedArg);
                }
            }
            // Adding fake input file to make TryValidate happy.
            arguments.Add(System.Reflection.Assembly.GetExecutingAssembly().Location);

            GeneratorOptions options = new GeneratorOptions
            {
                Arguments = arguments
            };
            IList<string> validationFailures;
            if (!options.TryValidate(out validationFailures))
            {
                response.Error += new InvalidOptionsException(validationFailures).Message;
                return;
            }

            Generator generator = Generator.CreateGenerator(options);
            generator.Generate(request, response);
        }

        public static int Main(string[] args)
        {
            // Hack to make sure everything's initialized
            DescriptorProtoFile.Descriptor.ToString();
            ExtensionRegistry extensionRegistry = ExtensionRegistry.CreateInstance();
            CSharpOptions.RegisterAllExtensions(extensionRegistry);

            CodeGeneratorRequest request;
            var response = new CodeGeneratorResponse.Builder();
            try
            {
                using (var input = Console.OpenStandardInput())
                {
                    request = CodeGeneratorRequest.ParseFrom(input, extensionRegistry);
                }
                Run(request, response);
            }
            catch (Exception e)
            {
                response.Error += e.ToString();
            }

            using (var output = Console.OpenStandardOutput())
            {
                response.Build().WriteTo(output);
                output.Flush();
            }
            return 0;
        }
    }
}
