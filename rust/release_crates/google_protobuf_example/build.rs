use protobuf_codegen::CodeGen;

fn main() {
    CodeGen::new()
        .inputs(["proto_example/foo.proto", "proto_example/bar/bar.proto"])
        .include("proto")
        .dependency(protobuf_well_known_types::get_dependency("protobuf_well_known_types"))
        .generate_and_compile()
        .unwrap();
}
