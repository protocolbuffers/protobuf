using Bee.NativeProgramSupport;
using Bee.CSharpSupport;
using Bee.Core;
using NiceIO;
using System;
using System.Collections.Generic;
using System.Linq;

class Build
{
    static NativeProgram Setup(IEnumerable<ToolChain> toolchains)
    {
        Backend.Current.ArtifactsPath = "bee/artifacts";

        var np = new NativeProgram("libprotobuf")
        {
            Sources = {
                // Protobuff lite
                "src/google/protobuf/any_lite.cc",
                "src/google/protobuf/arena.cc",
                "src/google/protobuf/extension_set.cc",
                "src/google/protobuf/generated_message_table_driven_lite.cc",
                "src/google/protobuf/generated_message_util.cc",
                "src/google/protobuf/implicit_weak_message.cc",
                "src/google/protobuf/io/coded_stream.cc",
                "src/google/protobuf/io/strtod.cc",
                "src/google/protobuf/io/io_win32.cc",
                "src/google/protobuf/io/zero_copy_stream.cc",
                "src/google/protobuf/io/zero_copy_stream_impl_lite.cc",
                "src/google/protobuf/message_lite.cc",
                "src/google/protobuf/parse_context.cc",
                "src/google/protobuf/repeated_field.cc",
                "src/google/protobuf/stubs/bytestream.cc",
                "src/google/protobuf/stubs/common.cc",
                "src/google/protobuf/stubs/int128.cc",
                "src/google/protobuf/stubs/status.cc",
                "src/google/protobuf/stubs/statusor.cc",
                "src/google/protobuf/stubs/stringpiece.cc",
                "src/google/protobuf/stubs/stringprintf.cc",
                "src/google/protobuf/stubs/structurally_valid.cc",
                "src/google/protobuf/stubs/strutil.cc",
                "src/google/protobuf/stubs/time.cc",
                "src/google/protobuf/wire_format_lite.cc",

                // Protobuff full
                "src/google/protobuf/any.cc",
                "src/google/protobuf/any.pb.cc",
                "src/google/protobuf/api.pb.cc",
                "src/google/protobuf/compiler/importer.cc",
                "src/google/protobuf/compiler/parser.cc",
                "src/google/protobuf/descriptor.cc",
                "src/google/protobuf/descriptor.pb.cc",
                "src/google/protobuf/descriptor_database.cc",
                "src/google/protobuf/duration.pb.cc",
                "src/google/protobuf/dynamic_message.cc",
                "src/google/protobuf/empty.pb.cc",
                "src/google/protobuf/extension_set_heavy.cc",
                "src/google/protobuf/field_mask.pb.cc",
                "src/google/protobuf/generated_message_reflection.cc",
                "src/google/protobuf/generated_message_table_driven.cc",
                "src/google/protobuf/io/gzip_stream.cc",
                "src/google/protobuf/io/printer.cc",
                "src/google/protobuf/io/tokenizer.cc",
                "src/google/protobuf/io/zero_copy_stream_impl.cc",
                "src/google/protobuf/map_field.cc",
                "src/google/protobuf/message.cc",
                "src/google/protobuf/reflection_ops.cc",
                "src/google/protobuf/service.cc",
                "src/google/protobuf/source_context.pb.cc",
                "src/google/protobuf/struct.pb.cc",
                "src/google/protobuf/stubs/mathlimits.cc",
                "src/google/protobuf/stubs/substitute.cc",
                "src/google/protobuf/text_format.cc",
                "src/google/protobuf/timestamp.pb.cc",
                "src/google/protobuf/type.pb.cc",
                "src/google/protobuf/unknown_field_set.cc",
                "src/google/protobuf/util/delimited_message_util.cc",
                "src/google/protobuf/util/field_comparator.cc",
                "src/google/protobuf/util/field_mask_util.cc",
                "src/google/protobuf/util/internal/datapiece.cc",
                "src/google/protobuf/util/internal/default_value_objectwriter.cc",
                //"src/google/protobuf/util/internal/error_listener.cc", // Empty file
                "src/google/protobuf/util/internal/field_mask_utility.cc",
                "src/google/protobuf/util/internal/json_escaping.cc",
                "src/google/protobuf/util/internal/json_objectwriter.cc",
                "src/google/protobuf/util/internal/json_stream_parser.cc",
                "src/google/protobuf/util/internal/object_writer.cc",
                "src/google/protobuf/util/internal/proto_writer.cc",
                "src/google/protobuf/util/internal/protostream_objectsource.cc",
                "src/google/protobuf/util/internal/protostream_objectwriter.cc",
                "src/google/protobuf/util/internal/type_info.cc",
                "src/google/protobuf/util/internal/type_info_test_helper.cc",
                "src/google/protobuf/util/internal/utility.cc",
                "src/google/protobuf/util/json_util.cc",
                "src/google/protobuf/util/message_differencer.cc",
                "src/google/protobuf/util/time_util.cc",
                "src/google/protobuf/util/type_resolver_util.cc",
                "src/google/protobuf/wire_format.cc",
                "src/google/protobuf/wrappers.pb.cc",
            }
        };

        np.IncludeDirectories.Add("src");
        np.Defines.Add("HAVE_PTHREAD=1");
        np.CompilerSettings().Add(s => s.WithRTTI(true));

        foreach (var toolchain in toolchains)
        {
            var nativeProgramConfiguration = new NativeProgramConfiguration(CodeGen.Release, toolchain, lump: true);

            var format = toolchain.StaticLibraryFormat;

            var deployedProgram = np.SetupSpecificConfiguration(nativeProgramConfiguration, format).DeployTo(GetBuildTargetDir(toolchain));
            Backend.Current.AddAliasDependency(toolchain.ActionName, deployedProgram.Path);

            if (toolchain.Platform is LinuxPlatform)
            {
                var toCopy = GetHeaderFilePaths(toolchains);
                Backend.Current.AddDependency(deployedProgram.Path, toCopy);
            }
        }

        return np;
    }

    static NPath GetBuildTargetDir(ToolChain toolchain)
    {
        var mapping = new Dictionary<string, string>
        {
            {"mac_x64", "macos/x64"},
            {"mac_arm64", "macos/arm64"},
            {"linux_x64_clang", "linux"},
        };

        var name = toolchain.ActionName.ToLowerInvariant();
        if (!mapping.ContainsKey(name))
            throw new ArgumentException($"Don't know output path for toolchain {name}");

        return $"builds/lib/{mapping[name]}";
    }

    static IEnumerable<ToolChain> GetToolchains()
    {
        var toolchains = new ToolChain[] {
            ToolChain.Store.Linux().Centos_7_7_1908().Clang_9_0_1().x64(),
            ToolChain.Store.Mac().Sdk_11_0().x64(),
            ToolChain.Store.Mac().Sdk_11_0().ARM64(),
        }.Where(t => t != null).ToArray();
        Console.WriteLine("Build Targets\n{0}", string.Join("\n", toolchains.Select(t => $"{t.ActionName.ToLowerInvariant()} {(t.CanBuild ? "" : " (can't build)")}")));

        return toolchains.Where(t => t.CanBuild).ToArray();
    }

    static IEnumerable<NPath> GetHeaderFilePaths(IEnumerable<ToolChain> toolchains)
    {
        var includeDir = "src/google/protobuf".ToNPath();
        var destinationDir = "builds/include/google/protobuf".ToNPath();

        var allHeaders = from f in includeDir.Files(recurse: true)
                         where f.Extension == "h" || f.Extension == "inc"
                         select CopyTool.Instance().Setup(destinationDir.Combine(f.RelativeTo(includeDir)), f);

        return allHeaders;
    }

    static void Main()
    {
        var toolchains = GetToolchains();
        Setup(toolchains);
    }
}
