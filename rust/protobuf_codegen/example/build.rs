use protobuf_codegen::CodeGen;
use std::env;

fn main() {
    let mut codegen = CodeGen::new();
    codegen
        .protoc_path(env::var("PROTOC").expect("PROTOC should be set to the path to protoc"))
        .protoc_gen_upb_minitable_path(env::var("PROTOC_GEN_UPB_MINITABLE").expect(
            "PROTOC_GEN_UPB_MINITABLE should be set to the path to protoc-gen-upb_minitable",
        ))
        .inputs(["foo.proto", "bar/bar.proto"])
        .include("proto");
    codegen.compile().unwrap();
}
