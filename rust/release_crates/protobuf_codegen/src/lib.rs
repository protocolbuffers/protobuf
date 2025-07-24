use std::fs::File;
use std::io::Write;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone)]
pub struct Dependency {
    pub crate_name: String,
    pub proto_import_paths: Vec<PathBuf>,
    pub proto_files: Vec<String>,
}

#[derive(Debug)]
pub struct CodeGen {
    inputs: Vec<PathBuf>,
    output_dir: PathBuf,
    includes: Vec<PathBuf>,
    dependencies: Vec<Dependency>,
}

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn missing_protoc_error_message() -> String {
    format!(
        "
Please make sure you have protoc available in your PATH. You can build it \
from source as follows: \
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
    let mut s = protoc_output.strip_prefix("libprotoc ").unwrap().trim().to_string();
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
    let mut s = cargo_version.replace("-rc.", "-rc");
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
            dependencies: Vec::new(),
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

    pub fn dependency(&mut self, deps: Vec<Dependency>) -> &mut Self {
        self.dependencies.extend(deps);
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

    fn generate_crate_mapping_file(&self) -> PathBuf {
        let crate_mapping_path = self.output_dir.join("crate_mapping.txt");
        let mut file = File::create(crate_mapping_path.clone()).unwrap();
        for dep in &self.dependencies {
            file.write_all(format!("{}\n", dep.crate_name).as_bytes()).unwrap();
            file.write_all(format!("{}\n", dep.proto_files.len()).as_bytes()).unwrap();
            for f in &dep.proto_files {
                file.write_all(format!("{}\n", f).as_bytes()).unwrap();
            }
        }
        crate_mapping_path
    }

    pub fn generate_and_compile(&self) -> Result<(), String> {
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
        for dep in &self.dependencies {
            for path in &dep.proto_import_paths {
                println!("cargo:rerun-if-changed={}", path.display());
            }
        }

        let crate_mapping_path = self.generate_crate_mapping_file();

        cmd.arg(format!("--rust_out={}", self.output_dir.display()))
            .arg("--rust_opt=experimental-codegen=enabled,kernel=upb");
        for include in &self.includes {
            cmd.arg(format!("--proto_path={}", include.display()));
        }
        for dep in &self.dependencies {
            for path in &dep.proto_import_paths {
                cmd.arg(format!("--proto_path={}", path.display()));
            }
        }
        cmd.arg(format!("--rust_opt=crate_mapping={}", crate_mapping_path.display()));
        let output = cmd.output().map_err(|e| format!("failed to run protoc: {}", e))?;
        println!("{}", std::str::from_utf8(&output.stdout).unwrap());
        eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
        assert!(output.status.success());

        for path in &self.expected_generated_rs_files() {
            if !path.exists() {
                return Err(format!("expected generated file {} does not exist", path.display()));
            }
            println!("cargo:rerun-if-changed={}", path.display());
        }

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
        assert_that!(protoc_version("libprotoc 30.0\n"), eq("30.0"));
        assert_that!(protoc_version("libprotoc 30.0-dev"), eq("30.0"));
        assert_that!(protoc_version("libprotoc 30.0-rc1"), eq("30.0-rc1"));
    }

    #[googletest::test]
    fn test_expected_protoc_version() {
        assert_that!(expected_protoc_version("4.30.0"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-alpha"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-beta"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-pre"), eq("30.0"));
        assert_that!(expected_protoc_version("4.30.0-rc.1"), eq("30.0-rc1"));
    }
}
