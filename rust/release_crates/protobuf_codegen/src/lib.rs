use std::path::{Path, PathBuf};

#[derive(Debug)]
pub struct CodeGen {
    inputs: Vec<PathBuf>,
    output_dir: PathBuf,
    includes: Vec<PathBuf>,
}

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn missing_protoc_error_message() -> String {
    format!(
        "
Please make sure you have protoc and protoc-gen-upb_minitable available in your PATH. You can \
build these binaries from source as follows: \
git clone https://github.com/protocolbuffers/protobuf.git; \
cd protobuf; \
git checkout rust-prerelease-{}; \
cmake . -Dprotobuf_FORCE_FETCH_DEPENDENCIES=ON; \
cmake --build . --parallel 12",
        VERSION
    )
}

// Given the output of "protoc --version", returns a shortened version string
// suitable for comparing against the protobuf crate version.
//
// The output of protoc --version looks something like "libprotoc XX.Y",
// optionally followed by "-dev" or "-rcN". We want to strip the "-dev" suffix
// if present and return something like "30.0" or "30.0-rc1".
fn protoc_version(protoc_output: &str) -> String {
    let mut s = protoc_output.strip_prefix("libprotoc ").unwrap().to_string();
    let first_dash = s.find("-dev");
    if let Some(i) = first_dash {
        s.truncate(i);
    }
    s
}

// Given a crate version string, returns just the part of it suitable for
// comparing against the protoc version. The crate version is of the form
// "X.Y.Z" with an optional suffix starting with a dash. We want to drop the
// major version ("X.") and only keep the suffix if it starts with "-rc".
fn expected_protoc_version(cargo_version: &str) -> String {
    let mut s = cargo_version.to_string();
    let is_release_candidate = s.find("-rc") != None;
    if !is_release_candidate {
        if let Some(i) = s.find('-') {
            s.truncate(i);
        }
    }
    let mut v: Vec<&str> = s.split('.').collect();
    assert_eq!(v.len(), 3);
    v.remove(0);
    v.join(".")
}

impl CodeGen {
    pub fn new() -> Self {
        Self {
            inputs: Vec::new(),
            output_dir: PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("protobuf_generated"),
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

    pub fn include(&mut self, include: impl AsRef<Path>) -> &mut Self {
        self.includes.push(include.as_ref().to_owned());
        self
    }

    pub fn includes(&mut self, includes: impl Iterator<Item = impl AsRef<Path>>) -> &mut Self {
        self.includes.extend(includes.into_iter().map(|include| include.as_ref().to_owned()));
        self
    }

    fn expected_generated_rs_files(&self) -> Vec<PathBuf> {
        self.inputs
            .iter()
            .map(|input| {
                let mut input = input.clone();
                assert!(input.set_extension("u.pb.rs"));
                self.output_dir.join(input)
            })
            .collect()
    }

    fn expected_generated_c_files(&self) -> Vec<PathBuf> {
        self.inputs
            .iter()
            .map(|input| {
                let mut input = input.clone();
                assert!(input.set_extension("upb_minitable.c"));
                self.output_dir.join(input)
            })
            .collect()
    }

    pub fn generate_and_compile(&self) -> Result<(), String> {
        let upb_version = std::env::var("DEP_UPB_VERSION").expect("DEP_UPB_VERSION should have been set, make sure that the Protobuf crate is a dependency");
        if VERSION != upb_version {
            panic!(
                "protobuf-codegen version {} does not match protobuf version {}.",
                VERSION, upb_version
            );
        }

        let mut version_cmd = std::process::Command::new("protoc");
        let output = version_cmd.arg("--version").output().map_err(|e| {
            format!("failed to run protoc --version: {} {}", e, missing_protoc_error_message())
        })?;

        let protoc_version = protoc_version(&String::from_utf8(output.stdout).unwrap());
        let expected_protoc_version = expected_protoc_version(VERSION);
        if protoc_version != expected_protoc_version {
            panic!(
                "Expected protoc version {} but found {}",
                expected_protoc_version, protoc_version
            );
        }

        // We cannot easily check the version of the minitable plugin, but let's at
        // least verify that it is present.
        std::process::Command::new("protoc-gen-upb_minitable")
            .stdin(std::process::Stdio::null())
            .output()
            .map_err(|e| {
                format!(
                    "Unable to find protoc-gen-upb_minitable: {} {}",
                    e,
                    missing_protoc_error_message()
                )
            })?;

        let mut cmd = std::process::Command::new("protoc");
        for input in &self.inputs {
            cmd.arg(input);
        }
        if !self.output_dir.exists() {
            // Attempt to make the directory if it doesn't exist
            let _ = std::fs::create_dir(&self.output_dir);
        }

        for include in &self.includes {
            println!("cargo:rerun-if-changed={}", include.display());
        }

        cmd.arg(format!("--rust_out={}", self.output_dir.display()))
            .arg("--rust_opt=experimental-codegen=enabled,kernel=upb")
            .arg(format!("--upb_minitable_out={}", self.output_dir.display()));
        for include in &self.includes {
            cmd.arg(format!("--proto_path={}", include.display()));
        }
        let output = cmd.output().map_err(|e| format!("failed to run protoc: {}", e))?;
        println!("{}", std::str::from_utf8(&output.stdout).unwrap());
        eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
        assert!(output.status.success());
        self.compile_only()
    }

    /// Builds and links the C code.
    pub fn compile_only(&self) -> Result<(), String> {
        let mut cc_build = cc::Build::new();
        cc_build
            .include(
                std::env::var_os("DEP_UPB_INCLUDE")
                    .expect("DEP_UPB_INCLUDE should have been set, make sure that the Protobuf crate is a dependency"),
            )
            .include(self.output_dir.clone())
            .flag("-std=c99");

        for path in &self.expected_generated_rs_files() {
            if !path.exists() {
                return Err(format!("expected generated file {} does not exist", path.display()));
            }
            println!("cargo:rerun-if-changed={}", path.display());
        }
        for path in &self.expected_generated_c_files() {
            if !path.exists() {
                return Err(format!("expected generated file {} does not exist", path.display()));
            }
            println!("cargo:rerun-if-changed={}", path.display());
            cc_build.file(path);
        }
        cc_build.compile(&format!("{}_upb_gen_code", std::env::var("CARGO_PKG_NAME").unwrap()));
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[gtest]
    fn test_protoc_version() {
        assert_that!(protoc_version("libprotoc 30.0"), eq("30.0"));
        assert_that!(protoc_version("libprotoc 30.0-dev"), eq("30.0"));
        assert_that!(protoc_version("libprotoc 30.0-rc1"), eq("30.0-rc1"));
    }

    #[googletest::test]
    fn test_expected_protoc_version() {
        assert_that!(expected_protoc_version("4.30.0"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-alpha"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-beta"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-pre"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-rc1"), eq("30.0-rc1"));
    }
}
