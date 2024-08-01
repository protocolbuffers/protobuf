use std::fs::{self, OpenOptions};
use std::io::{self, prelude::*};
use std::path::{Path, PathBuf};
use walkdir::WalkDir;

#[derive(Debug)]
pub struct CodeGen {
    inputs: Vec<PathBuf>,
    output_dir: PathBuf,
    protoc_path: PathBuf,
    protoc_gen_upb_path: PathBuf,
    protoc_gen_upb_minitable_path: PathBuf,
    includes: Vec<PathBuf>,
}

const VERSION: &str = env!("CARGO_PKG_VERSION");

impl CodeGen {
    pub fn new() -> Self {
        Self {
            inputs: Vec::new(),
            output_dir: std::env::current_dir().unwrap().join("src").join("protos"),
            protoc_path: PathBuf::from("protoc"),
            protoc_gen_upb_path: PathBuf::from("protoc-gen-upb"),
            protoc_gen_upb_minitable_path: PathBuf::from("protoc-gen-upb_minitable"),
            includes: Vec::new(),
        }
    }

    pub fn input(&mut self, input: impl AsRef<Path>) -> &mut Self {
        self.inputs.push(input.as_ref().to_owned());
        self
    }

    pub fn inputs(&mut self, inputs: impl IntoIterator<Item = impl AsRef<Path>>) -> &mut Self {
        self.inputs.extend(inputs.into_iter().map(|input| input.as_ref().to_owned()));
        self
    }

    pub fn output_dir(&mut self, output_dir: impl AsRef<Path>) -> &mut Self {
        self.output_dir = output_dir.as_ref().to_owned();
        self
    }

    pub fn protoc_path(&mut self, protoc_path: impl AsRef<Path>) -> &mut Self {
        self.protoc_path = protoc_path.as_ref().to_owned();
        self
    }

    pub fn protoc_gen_upb_path(&mut self, protoc_gen_upb_path: impl AsRef<Path>) -> &mut Self {
        self.protoc_gen_upb_path = protoc_gen_upb_path.as_ref().to_owned();
        self
    }

    pub fn protoc_gen_upb_minitable_path(
        &mut self,
        protoc_gen_upb_minitable_path: impl AsRef<Path>,
    ) -> &mut Self {
        self.protoc_gen_upb_minitable_path = protoc_gen_upb_minitable_path.as_ref().to_owned();
        self
    }

    pub fn include(&mut self, include: impl AsRef<Path>) -> &mut Self {
        self.includes.push(include.as_ref().to_owned());
        self
    }

    pub fn includes(&mut self, includes: impl Iterator<Item = impl AsRef<Path>>) -> &mut Self {
        self.includes.extend(includes.into_iter().map(|include| include.as_ref().to_owned()));
        self
    }

    /// Generate Rust, upb, and upb minitables. Build and link the C code.
    pub fn compile(&self) -> Result<(), String> {
        let libupb_version = std::env::var("DEP_LIBUPB_VERSION").expect("DEP_LIBUPB_VERSION should have been set, make sure that the Protobuf crate is a dependency");
        if VERSION != libupb_version {
            panic!(
                "protobuf-codegen version {} does not match protobuf version {}.",
                VERSION, libupb_version
            );
        }

        let mut cmd = std::process::Command::new(&self.protoc_path);
        for input in &self.inputs {
            cmd.arg(input);
        }
        cmd.arg(format!("--rust_out={}", self.output_dir.display()))
            .arg("--rust_opt=experimental-codegen=enabled,kernel=upb")
            .arg(format!("--plugin=protoc-gen-upb={}", self.protoc_gen_upb_path.display()))
            .arg(format!(
                "--plugin=protoc-gen-upb_minitable={}",
                self.protoc_gen_upb_minitable_path.display()
            ))
            .arg(format!("--upb_minitable_out={}", self.output_dir.display()))
            .arg(format!("--upb_out={}", self.output_dir.display()));
        for include in &self.includes {
            cmd.arg(format!("--proto_path={}", include.display()));
        }
        let output = cmd.output().map_err(|e| format!("failed to run protoc: {}", e))?;
        println!("{}", std::str::from_utf8(&output.stdout).unwrap());
        eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
        assert!(output.status.success());

        let mut cc_build = cc::Build::new();
        cc_build
            .include(
                std::env::var_os("DEP_LIBUPB_INCLUDE")
                    .expect("DEP_LIBUPB_INCLUDE should have been set, make sure that the Protobuf crate is a dependency"),
            )
            .include(self.output_dir.clone())
            .flag("-std=c99");
        for entry in WalkDir::new(&self.output_dir) {
            if let Ok(entry) = entry {
                let path = entry.path();
                let file_name = path.file_name().unwrap().to_str().unwrap();
                if file_name.ends_with(".upb_minitable.c") {
                    cc_build.file(path);
                }
                if file_name.ends_with(".upb.h") {
                    Self::strip_upb_inline(&path);
                    cc_build.file(path.with_extension("c"));
                }
                if file_name.ends_with(".pb.rs") {
                    Self::fix_rs_gencode(&path);
                }
            }
        }
        cc_build.compile(&format!("{}_upb_gen_code", std::env::var("CARGO_PKG_NAME").unwrap()));
        Ok(())
    }

    // Remove UPB_INLINE from the .upb.h file.
    fn strip_upb_inline(header: &Path) {
        let contents = fs::read_to_string(header).unwrap().replace("UPB_INLINE ", "");
        let mut file =
            OpenOptions::new().write(true).truncate(true).open(header.with_extension("c")).unwrap();
        file.write(contents.as_bytes()).unwrap();
    }

    // Adjust the generated Rust code to work with the crate structure.
    fn fix_rs_gencode(path: &Path) {
        let contents = fs::read_to_string(path)
            .unwrap()
            .replace("crate::", "")
            .replace("protobuf_upb", "protobuf")
            .replace("::__pb", "__pb")
            .replace("::__std", "__std");
        let mut file = OpenOptions::new().write(true).truncate(true).open(path).unwrap();
        file.write(contents.as_bytes()).unwrap();
    }
}
