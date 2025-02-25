use protobuf_codegen::CodeGen;
use protobuf_codegen::proto_dep;

fn main() {
    CodeGen::new()
        .inputs(["proto_example/foo.proto", "proto_example/bar/bar.proto"])
        .include("proto")
        .dependency(proto_dep!(protobuf_well_known_types))
        .generate_and_compile()
        .unwrap();
}
