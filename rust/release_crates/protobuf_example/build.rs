use protobuf_codegen::CodeGen;

fn main() {
    let mut codegen = CodeGen::new();
    codegen.inputs(["foo.proto", "bar/bar.proto"]).include("proto");
    codegen.compile().unwrap();
}
