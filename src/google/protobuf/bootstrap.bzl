"""BUILD rules for bootstrapping protos"""

def bootstrap_proto(
        name,
        outs,
        proto,
        deps = [],
        has_services = False,
        **kwargs):
    """Bootstrap proto with the protoc cpp gencode

    Args:
      name: A name for the target.
      outs: The paths to write the output gencode file(s).
      proto: The proto to bootstrap.
      deps: The deps of the proto to bootstrap.
      has_services: Indicates whether this proto has service definitions.
      **kwargs: any other rule arguments.
    """

    args = ""
    extra_tools = []
    cpp_out_options = ["bootstrap"]

    native.genrule(
        name = name,
        srcs = deps + [proto],
        outs = outs,
        cmd =
            "src=$(location " + proto + ")\n" +
            "proto_path=''\n" +
            "deps=" + ",".join(["$(location " + dep + ")" for dep in deps]) + "\n" +
            "if [[ $$src == $(GENDIR)* ]]; then\n" +
            "  proto_path='$(GENDIR)'\n" +
            "  for dep in $$deps; do\n" +
            "    mkdir -p $(GENDIR)/$$(dirname $$dep) && cp $$dep $(GENDIR)/$$dep\n" +
            "  done\n" +
            "fi\n" +
            "$(location //src/google/protobuf/compiler:protoc_minimal) " +
            "  --plugin=protoc-gen-cpp=$(location //src/google/protobuf/compiler/cpp:protoc-gen-cpp) " +
            "  --proto_path=$$proto_path " +
            "  --cpp_out=" + ",".join(cpp_out_options) + ":$(GENDIR) " +
            args + " $$src\n" +
            "for out in $(OUTS); do\n" +
            "  if [[ $(RULEDIR) != $$(dirname $$out) ]]; then\n" +
            "    cp $(RULEDIR)/$$(basename $$out) $$out\n" +
            "  fi\n" +
            "done",
        visibility = ["//net/proto2/compiler/cpp/internal:__pkg__"],
        tools = extra_tools + ["//src/google/protobuf/compiler:protoc_minimal", "//src/google/protobuf/compiler/cpp:protoc-gen-cpp"],
        **kwargs
    )
