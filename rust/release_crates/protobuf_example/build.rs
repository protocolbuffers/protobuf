use protobuf_codegen::CodeGen;

fn main() {
    CodeGen::new()
        .inputs(["foo.proto", "bar/bar.proto"])
        .include("proto")
        .generate_and_compile()
        .unwrap();
}
