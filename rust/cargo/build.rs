fn main() {
    cc::Build::new()
        .flag("-std=c99")
        // TODO: Come up with a way to enable lto
        // .flag("-flto=thin")
        .warnings(false)
        .include("src")
        .file("src/upb.c")
        .file("src/utf8_range.c")
        .file("src/upb/upb_api.c")
        .define("UPB_BUILD_API", Some("1"))
        .define("PROTOBUF_CARGO", Some("1"))
        .compile("upb");
    let path = std::path::Path::new("src");
    println!("cargo:include={}", path.canonicalize().unwrap().display())
}
