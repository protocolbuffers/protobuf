use protobuf_codegen::CodeGen;

fn main() {
    let mut codegen = CodeGen::new();
    codegen.inputs(["foo.proto", "bar/bar.proto"]).include("proto");
    if std::env::var("CARGO_FEATURE_RUN_PROTOBUF_CODEGEN").is_ok() {
        codegen.generate_and_compile().unwrap();
    } else {
        codegen.compile_only().unwrap();
    }
}
