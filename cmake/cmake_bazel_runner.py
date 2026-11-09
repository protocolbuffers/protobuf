# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Helper run by each action of the `cmake_bazel()` rule (cmake_bazel.bzl).

Subcommands, one per action kind:

  configure  Runs `cmake -G Ninja` over the CMake files plus empty stubs for
             every other source file, and records the exact compile and link
             commands Ninja would run in a JSON spec.
  codegen    Runs CMake's custom commands that generate sources (e.g. protoc).
  compile    Runs the recorded compile command(s) for one source file.
  scan       Finds the headers each source file includes (the compiler's -M)
             and writes a per-source header bundle with exactly those files.
  compile_generated
             Runs the recorded compile commands for all generated sources.
  link       Replays the recorded archive/link commands over all objects.
  package    Runs `cmake --install`, split into include/ and everything else.

Each action works in a private scratch directory laid out the same way:
<work>/src (the source tree), <work>/deps/<name> (dependency install trees) and
<work>/build (the build tree, and the working directory of every replayed
command). The recorded compile and link commands refer to these with relative
paths (../src, ../deps, .), so they are identical on every run and machine --
for Bazel's cache, and for compiler caches such as sccache. Any other absolute
path that differs between actions (the scratch directory itself, Bazel's
execution root) is replaced by a placeholder token and restored by the
consuming action.

The configure action writes one compile spec per source file, so a
re-configure that leaves a file's compile command unchanged leaves its compile
action cached.
"""

import argparse
import concurrent.futures
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile

# Placeholder tokens for per-action absolute paths.
WORK_TOKEN = "@CMAKE_BAZEL_WORK@"
EXECROOT_TOKEN = "@CMAKE_BAZEL_EXECROOT@"

# Prefixes of execroot-relative paths in compiler/linker flags coming from the
# Bazel C++ toolchain. CMake runs the compiler from its build directory, so
# such paths are made absolute.
_EXECROOT_RELATIVE_PREFIXES = ("external/", "bazel-out/")
_PATH_FLAG_PREFIXES = ("-I", "-iquote", "-isystem", "-idirafter", "-B", "-L",
                       "--sysroot=", "-fsanitize-ignorelist=",
                       "-fsanitize-blacklist=")

# Files in the CMake build tree that are not needed after configure and would
# make the build-tree archive depend on the machine it was produced on.
_BUILD_TREE_EXCLUDES = ("build.ninja", "rules.ninja", ".ninja_log",
                        ".ninja_deps", "compile_commands.json",
                        "CMakeConfigureLog.yaml", "CMakeOutput.log",
                        "CMakeError.log")
_GENERATED_SOURCE_EXTENSIONS = (".c", ".cc", ".cpp", ".cxx")
_BUILD_TREE_EXCLUDED_DIR_PARTS = ("CMakeScratch", "CompilerIdC",
                                  "CompilerIdCXX")


def _log(msg):
  print("cmake_bazel: " + msg, file=sys.stderr)


def _abs_flag(flag, execroot):
  """Makes an execroot-relative path in a toolchain flag absolute."""
  if flag.startswith(_EXECROOT_RELATIVE_PREFIXES):
    return os.path.join(execroot, flag)
  for prefix in _PATH_FLAG_PREFIXES:
    rest = flag[len(prefix):]
    if flag.startswith(prefix) and rest.startswith(_EXECROOT_RELATIVE_PREFIXES):
      return prefix + os.path.join(execroot, rest)
  return flag


def _abs_tool(path, execroot):
  return path if os.path.isabs(path) else os.path.join(execroot, path)


def _tokenize(text, work, execroot):
  # Replace the longer path first in case one contains the other.
  for path, token in sorted(((work, WORK_TOKEN), (execroot, EXECROOT_TOKEN)),
                            key=lambda p: -len(p[0])):
    text = text.replace(path, token)
  return text


def _relativize(text, work, execroot):
  """Rewrites a command recorded in <work>/build to use relative paths."""
  for path, rel in ((work + "/src", "../src"), (work + "/deps", "../deps"),
                    (work + "/build", ".")):
    text = re.sub(re.escape(path) + r"(?=[/\s\"';:=]|$)", rel, text)
  return _tokenize(text, work, execroot)


def _detokenize(text, work, execroot):
  return text.replace(WORK_TOKEN, work).replace(EXECROOT_TOKEN, execroot)


def _stage_files(execroot, root, mappings, copy=False):
  """Places each `exec_path=rel_path` mapping at root/rel_path."""
  for mapping in mappings:
    exec_path, rel = mapping.split("=", 1)
    dst = os.path.join(root, rel)
    if os.path.lexists(dst):
      continue
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    src = os.path.join(execroot, exec_path)
    if copy:
      shutil.copyfile(src, dst)
      shutil.copymode(src, dst)
    else:
      os.symlink(src, dst)


def _stage_deps(execroot, work, mappings):
  """Assembles each dependency install prefix at <work>/deps/<name>.

  Each mapping is `name=include_tree=rest_tree`: the prefix's include/
  directory and everything else (lib/, bin/, ...) are separate tree artifacts,
  so that compile actions can depend on headers only. An empty rest_tree
  stages headers only. Returns the staged prefixes, for CMAKE_PREFIX_PATH.
  """
  prefixes = []
  for mapping in mappings:
    name, include_tree, rest_tree = mapping.split("=", 2)
    prefix = os.path.join(work, "deps", name)
    os.makedirs(prefix)
    os.symlink(os.path.join(execroot, include_tree),
               os.path.join(prefix, "include"))
    if rest_tree:
      rest = os.path.join(execroot, rest_tree)
      for entry in sorted(os.listdir(rest)):
        os.symlink(os.path.join(rest, entry), os.path.join(prefix, entry))
    prefixes.append(prefix)
  return prefixes


def _make_dirs(build_dir, dirs):
  for d in dirs:
    os.makedirs(os.path.join(build_dir, d), exist_ok=True)


def _extract(tar_path, dest):
  with tarfile.open(tar_path) as tar:
    tar.extractall(dest)
    return tar.getnames()


def _deterministic_tar(out_tar, base_dir, rel_paths):
  """Writes rel_paths (relative to base_dir) into a reproducible tar."""

  def _reset(info):
    info.mtime = 0
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    return info

  with tarfile.open(out_tar, "w", format=tarfile.GNU_FORMAT) as tar:
    for rel in sorted(rel_paths):
      tar.add(os.path.join(base_dir, rel), arcname=rel, recursive=False,
              filter=_reset)


def _run(cmd, cwd, env=None):
  result = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str))
  if result.returncode != 0:
    _log("command failed (exit %d) in %s:\n  %s" %
         (result.returncode, cwd, cmd if isinstance(cmd, str) else
          " ".join(cmd)))
    sys.exit(result.returncode)


def _read_args_file(path):
  with open(path) as f:
    return [line.rstrip("\n") for line in f if line.rstrip("\n")]


################################################################################
# configure
################################################################################


def _cmd_configure(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_cfg_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)

  _stage_files(execroot, src_dir, args.cmake_src, copy=True)

  # Every other source file exists only as an empty stub: CMake needs the
  # files it adds to targets to exist, but never reads them during
  # configuration. This keeps source edits out of this action's inputs.
  for rel in _read_args_file(args.stubs):
    path = os.path.join(src_dir, rel)
    if not os.path.exists(path):
      os.makedirs(os.path.dirname(path), exist_ok=True)
      open(path, "w").close()

  cmake = _abs_tool(args.cmake, execroot)
  ninja = _abs_tool(args.ninja, execroot)

  # Strip the per-action paths from what the compiler embeds in its outputs
  # (debug info, __FILE__), so objects are reproducible across actions.
  prefix_maps = [
      "-ffile-prefix-map=%s=/cmake_bazel" % work,
      "-ffile-prefix-map=%s=/execroot" % execroot,
  ]
  cflags = [_abs_flag(f, execroot) for f in args.cflag] + prefix_maps
  cxxflags = [_abs_flag(f, execroot) for f in args.cxxflag] + prefix_maps
  ldflags = [_abs_flag(f, execroot) for f in args.ldflag]

  prefix_path = ";".join(_stage_deps(execroot, work, args.dep_dir))

  cmd = [
      cmake,
      "-S", src_dir,
      "-B", build_dir,
      "-G", "Ninja",
      "-Wno-dev",
      "-DCMAKE_MAKE_PROGRAM=" + ninja,
      "-DCMAKE_C_COMPILER=" + _abs_tool(args.cc, execroot),
      "-DCMAKE_CXX_COMPILER=" + _abs_tool(args.cxx, execroot),
      "-DCMAKE_AR=" + _abs_tool(args.ar, execroot),
      "-DCMAKE_C_FLAGS=" + " ".join(cflags),
      "-DCMAKE_CXX_FLAGS=" + " ".join(cxxflags),
      "-DCMAKE_EXE_LINKER_FLAGS=" + " ".join(ldflags),
      "-DCMAKE_SHARED_LINKER_FLAGS=" + " ".join(ldflags),
      "-DCMAKE_MODULE_LINKER_FLAGS=" + " ".join(ldflags),
      "-DCMAKE_PREFIX_PATH=" + prefix_path,
      "-DCMAKE_INSTALL_PREFIX=" + os.path.join(work, "install"),
      "-DCMAKE_INSTALL_LIBDIR=lib",
      # The build tree is never re-generated; Ninja must not try to.
      "-DCMAKE_SUPPRESS_REGENERATION=ON",
  ] + ["-D" + entry for entry in args.cache_entry]
  _run(cmd, cwd=work)

  # Compile commands, keyed by source path relative to the source root. The
  # CMake Ninja generator names its compile rules <LANG>_COMPILER__<target>...
  compile_rules = [
      rule for rule in subprocess.check_output(
          [ninja, "-C", build_dir, "-t", "rules"], text=True).split()
      if "_COMPILER__" in rule
  ]
  compdb = json.loads(
      subprocess.check_output(
          [ninja, "-C", build_dir, "-t", "compdb"] + compile_rules,
          text=True)) if compile_rules else []
  compile_cmds = {}
  compile_cmd_set = set()
  generated_compile_cmds = []
  unowned = []
  for entry in compdb:
    src = os.path.normpath(os.path.join(entry["directory"], entry["file"]))
    command = entry["command"]
    compile_cmd_set.add(command)
    if src.startswith(build_dir + os.sep):
      # A source generated during the build (by a custom command).
      generated_compile_cmds.append(
          [entry["output"], _relativize(command, work, execroot)])
      continue
    if not src.startswith(src_dir + os.sep):
      unowned.append(src)
      continue
    rel = os.path.relpath(src, src_dir)
    compile_cmds.setdefault(rel, []).append(
        [entry["output"], _relativize(command, work, execroot)])

  if unowned:
    _log("CMake compiles files outside the source and build trees, which "
         "cmake_bazel does not support:\n  " + "\n  ".join(sorted(unowned)))
    sys.exit(1)

  # Custom commands (e.g. protoc runs from protobuf_generate()).
  custom_cmd_set = set()
  if "CUSTOM_COMMAND" in subprocess.check_output(
      [ninja, "-C", build_dir, "-t", "rules"], text=True).split():
    custom_cmd_set = {
        entry["command"] for entry in json.loads(subprocess.check_output(
            [ninja, "-C", build_dir, "-t", "compdb", "CUSTOM_COMMAND"],
            text=True))
    }

  # Every compiled source must have a compile action; otherwise part of the
  # CMake build would silently be skipped.
  spec_outputs = dict(
      line.split("=", 1) for line in _read_args_file(args.spec_map))
  missing = sorted(set(compile_cmds) - set(spec_outputs))
  if missing:
    _log("CMake compiles these files, but no compile action covers them "
         "(add them to `srcs`):\n  " + "\n  ".join(missing))
    sys.exit(1)

  # Everything else Ninja would run, in build order: archiving, linking and
  # library symlinks.
  link_cmds = []
  codegen_cmds = []
  for command in subprocess.check_output(
      [ninja, "-C", build_dir, "-t", "commands"], text=True).splitlines():
    if command and command not in compile_cmd_set:
      if " @" in command:
        _log("response files are not supported: " + command)
        sys.exit(1)
      if command in custom_cmd_set:
        codegen_cmds.append(_relativize(command, work, execroot))
      else:
        link_cmds.append(_relativize(command, work, execroot))

  # Directories the build writes into. Ninja creates an output's directory
  # before running its command; the replayed commands do not.
  # Directories CMake created at configure time (file(MAKE_DIRECTORY)) are
  # recreated too.
  output_dirs = {
      os.path.relpath(os.path.join(root, d), build_dir)
      for root, dirs, _ in os.walk(build_dir)
      for d in dirs
  }
  output_dirs = {
      d for d in output_dirs
      if not any(p in d for p in _BUILD_TREE_EXCLUDED_DIR_PARTS)
  }
  for line in subprocess.check_output(
      [ninja, "-C", build_dir, "-t", "targets", "all"], text=True).splitlines():
    output, _, rule = line.rpartition(": ")
    if output and rule != "phony" and not os.path.isabs(output):
      if os.path.dirname(output):
        output_dirs.add(os.path.dirname(output))

  for rel, out in spec_outputs.items():
    with open(os.path.join(execroot, out), "w") as f:
      json.dump(compile_cmds.get(rel, []), f, indent=1)
  with open(args.out_link_spec, "w") as f:
    json.dump({"link": link_cmds, "output_dirs": sorted(output_dirs)}, f,
              indent=1, sort_keys=True)
  with open(args.out_gen_spec, "w") as f:
    json.dump({"codegen": codegen_cmds, "compile": generated_compile_cmds,
               "output_dirs": sorted(output_dirs)}, f, indent=1,
              sort_keys=True)

  # The build tree is what `cmake --install` needs later (cmake_install.cmake
  # files, generated package config files).
  rel_paths = []
  for root, dirs, files in os.walk(build_dir):
    dirs[:] = [d for d in dirs
               if not any(p in d for p in _BUILD_TREE_EXCLUDED_DIR_PARTS)]
    for name in files:
      if name in _BUILD_TREE_EXCLUDES or name.endswith(".bin"):
        continue
      path = os.path.join(root, name)
      with open(path, "rb") as f:
        data = f.read()
      if b"\0" not in data:
        text = data.decode("utf-8", errors="surrogateescape")
        with open(path, "wb") as f:
          f.write(_tokenize(text, work, execroot).encode(
              "utf-8", errors="surrogateescape"))
      rel_paths.append(os.path.relpath(path, build_dir))
  _deterministic_tar(args.out_build_tree, build_dir, rel_paths)
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# compile
################################################################################


def _stage_bundle(execroot, work, bundle):
  """Places every file of a header bundle (see `scan`) at work/<its path>."""
  bundle = os.path.join(execroot, bundle)
  for root, _, files in os.walk(bundle):
    for name in files:
      src = os.path.join(root, name)
      dst = os.path.join(work, os.path.relpath(src, bundle))
      os.makedirs(os.path.dirname(dst), exist_ok=True)
      os.symlink(src, dst)


def _cmd_compile(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_cc_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  _stage_files(execroot, src_dir, [args.src])
  if args.headers:
    _stage_files(execroot, src_dir, _read_args_file(args.headers))
  if args.bundle:
    # Exactly the headers this source includes, found by the scan action:
    # checked-in, generated and scanned dependency headers.
    _stage_bundle(execroot, work, args.bundle)
  _stage_deps(execroot, work, args.dep_dir)
  if args.gen_tar:
    # Sources may include headers generated during the build.
    _extract(args.gen_tar, build_dir)

  with open(args.spec) as f:
    commands = json.load(f)

  outputs = []
  for output, command in commands:
    os.makedirs(os.path.join(build_dir, os.path.dirname(output)),
                exist_ok=True)
    _run(_detokenize(command, work, execroot), cwd=build_dir)
    outputs.append(output)

  # A source CMake does not compile (in this configuration) yields an empty
  # archive.
  _deterministic_tar(args.out_tar, build_dir, outputs)
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# scan
################################################################################


def _scan_command(command, depfile):
  """Turns a recorded compile command into one that only lists its includes.

  The command keeps every flag that affects preprocessing (include paths,
  defines, language standard), so the header search resolves exactly as in
  the compile. Its object and dependency-file outputs are replaced by `-M`
  into `depfile`.
  """
  tokens = shlex.split(command)
  out = []
  skip_next = False
  for token in tokens:
    if skip_next:
      skip_next = False
      continue
    if token in ("-o", "-MF", "-MT", "-MQ"):
      skip_next = True
      continue
    if token in ("-c", "-MD", "-MMD", "-MP"):
      continue
    out.append(token)
  return out + ["-M", "-MF", depfile]


def _parse_depfile(path):
  """Returns the prerequisites listed in a Make-style dependency file."""
  with open(path) as f:
    text = f.read().replace("\\\n", " ")
  _, _, rest = text.partition(": ")
  return [
      re.sub(r"\\([ #\\])", r"\1", token).replace("$$", "$")
      for token in re.findall(r"(?:\\.|[^\s\\])+", rest)
  ]


def _cmd_scan(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_scan_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  depfile_dir = os.path.join(work, "depfiles")
  os.makedirs(build_dir)
  os.makedirs(depfile_dir)

  # rel_path=src_exec_path=spec_exec_path=bundle_exec_path, one per source.
  entries = [line.split("=", 3) for line in _read_args_file(args.sources)]
  _stage_files(execroot, src_dir,
               ["%s=%s" % (src, rel) for rel, src, _, _ in entries] +
               _read_args_file(args.headers))
  _stage_deps(execroot, work, args.dep_dir)
  if args.gen_tar:
    _extract(args.gen_tar, build_dir)
  bundled_deps = set(args.bundle_dep)

  def bundle_path(path):
    """Returns path relative to work if it belongs in a bundle, else None."""
    if not os.path.isabs(path):
      path = os.path.normpath(os.path.join(build_dir, path))
    if not path.startswith(work + os.sep):
      return None  # System and toolchain headers come with the toolchain.
    rel = os.path.relpath(path, work)
    parts = rel.split(os.sep)
    if parts[0] in ("src", "build"):
      return rel
    if parts[0] == "deps" and len(parts) > 2 and parts[1] in bundled_deps:
      return rel
    return None  # Other dependencies are inputs of every compile anyway.

  def scan_one(index_and_entry):
    index, (rel, _, spec, bundle) = index_and_entry
    with open(os.path.join(execroot, spec)) as f:
      commands = json.load(f)
    found = set()
    for i, (_, command) in enumerate(commands):
      depfile = os.path.join(depfile_dir, "%d.%d.d" % (index, i))
      cmd = _scan_command(_detokenize(command, work, execroot), depfile)
      result = subprocess.run(cmd, cwd=build_dir)
      if result.returncode != 0:
        return "%s: scanning failed (exit %d):\n  %s" % (
            rel, result.returncode, " ".join(cmd))
      for path in _parse_depfile(depfile):
        path = bundle_path(path)
        if path and path != os.path.join("src", rel):
          found.add(path)
    out = os.path.join(execroot, bundle)
    os.makedirs(out, exist_ok=True)
    for path in sorted(found):
      dst = os.path.join(out, path)
      os.makedirs(os.path.dirname(dst), exist_ok=True)
      shutil.copyfile(os.path.join(work, path), dst)
      os.chmod(dst, 0o644)
    return None

  with concurrent.futures.ThreadPoolExecutor(
      max_workers=os.cpu_count() or 2) as pool:
    errors = [e for e in pool.map(scan_one, enumerate(entries)) if e]
  if errors:
    _log("\n".join(errors))
    sys.exit(1)
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# codegen
################################################################################


def _cmd_codegen(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_gen_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  _stage_files(execroot, src_dir, _read_args_file(args.src_map))
  _stage_deps(execroot, work, args.dep_dir)

  with open(args.spec) as f:
    spec = json.load(f)
  _make_dirs(build_dir, spec["output_dirs"])
  for command in spec["codegen"]:
    _run(_detokenize(command, work, execroot), cwd=build_dir)

  # Generated sources and everything else (headers) separately: per-file
  # compiles of checked-in sources only need the generated headers.
  rel_paths = [
      os.path.relpath(os.path.join(root, name), build_dir)
      for root, _, files in os.walk(build_dir)
      for name in files
  ]
  sources = [r for r in rel_paths if r.endswith(_GENERATED_SOURCE_EXTENSIONS)]
  _deterministic_tar(args.out_sources_tar, build_dir, sources)
  _deterministic_tar(args.out_headers_tar, build_dir,
                     [r for r in rel_paths if r not in set(sources)])
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# compile_generated
################################################################################


def _cmd_compile_generated(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_gencc_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  _stage_files(execroot, src_dir, _read_args_file(args.headers))
  _stage_deps(execroot, work, args.dep_dir)
  _extract(args.gen_headers_tar, build_dir)
  _extract(args.gen_sources_tar, build_dir)

  with open(args.spec) as f:
    commands = json.load(f)["compile"]

  def compile_one(item):
    output, command = item
    os.makedirs(os.path.join(build_dir, os.path.dirname(output)),
                exist_ok=True)
    _run(_detokenize(command, work, execroot), cwd=build_dir)
    return output

  with concurrent.futures.ThreadPoolExecutor(
      max_workers=os.cpu_count() or 2) as pool:
    outputs = list(pool.map(compile_one, commands))
  _deterministic_tar(args.out_tar, build_dir, outputs)
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# link
################################################################################


def _cmd_link(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_link_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  # Link commands can read source files (e.g. linker version scripts).
  _stage_files(execroot, src_dir, args.cmake_src)

  objects = set()
  for obj_tar in _read_args_file(args.obj_tars):
    with tarfile.open(obj_tar) as tar:
      objects.update(tar.getnames())
      tar.extractall(build_dir)

  _stage_deps(execroot, work, args.dep_dir)

  with open(args.spec) as f:
    spec = json.load(f)
  for d in spec["output_dirs"]:
    os.makedirs(os.path.join(build_dir, d), exist_ok=True)
  for command in spec["link"]:
    _run(_detokenize(command, work, execroot), cwd=build_dir)

  rel_paths = []
  for root, _, files in os.walk(build_dir):
    for name in files:
      rel = os.path.relpath(os.path.join(root, name), build_dir)
      if rel not in objects and not name.endswith(".d"):
        rel_paths.append(rel)
  _deterministic_tar(args.out_tar, build_dir, rel_paths)

  # Executables requested as individual outputs (e.g. test binaries).
  for mapping in args.executable:
    rel, out = mapping.split("=", 1)
    src = os.path.join(build_dir, rel)
    if not os.path.isfile(src):
      _log("executable %s was not built" % rel)
      sys.exit(1)
    shutil.copy2(src, os.path.join(execroot, out))
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# package
################################################################################


def _cmd_package(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_pkg_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  # Copies, not symlinks: `cmake --install` would install symlinks as such.
  _stage_files(execroot, src_dir, _read_args_file(args.src_map), copy=True)
  _stage_deps(execroot, work, args.dep_dir)

  with tarfile.open(args.build_tree) as tar:
    tar.extractall(build_dir)
  for root, _, files in os.walk(build_dir):
    for name in files:
      path = os.path.join(root, name)
      with open(path, "rb") as f:
        data = f.read()
      if b"\0" not in data and (WORK_TOKEN.encode() in data or
                                EXECROOT_TOKEN.encode() in data):
        text = data.decode("utf-8", errors="surrogateescape")
        with open(path, "wb") as f:
          f.write(_detokenize(text, work, execroot).encode(
              "utf-8", errors="surrogateescape"))
  with tarfile.open(args.libs_tar) as tar:
    tar.extractall(build_dir)

  out_include = os.path.join(execroot, args.out_include)
  out_rest = os.path.join(execroot, args.out_rest)
  os.makedirs(out_include, exist_ok=True)
  os.makedirs(out_rest, exist_ok=True)
  # Install into the "rest" output, then move include/ into its own output.
  _run([_abs_tool(args.cmake, execroot), "--install", build_dir, "--prefix",
        out_rest], cwd=work)
  include = os.path.join(out_rest, "include")
  if os.path.isdir(include):
    for entry in os.listdir(include):
      shutil.move(os.path.join(include, entry), out_include)
    os.rmdir(include)
  shutil.rmtree(work, ignore_errors=True)


################################################################################
# main
################################################################################


def _add_toolchain_args(p):
  p.add_argument("--cc", required=True)
  p.add_argument("--cxx", required=True)
  p.add_argument("--ar", required=True)
  p.add_argument("--cflag", action="append", default=[])
  p.add_argument("--cxxflag", action="append", default=[])
  p.add_argument("--ldflag", action="append", default=[])


def main(argv):
  parser = argparse.ArgumentParser(fromfile_prefix_chars="@")
  sub = parser.add_subparsers(dest="command", required=True)

  p = sub.add_parser("configure")
  _add_toolchain_args(p)
  p.add_argument("--cmake", required=True)
  p.add_argument("--ninja", required=True)
  p.add_argument("--cmake-src", action="append", default=[])
  p.add_argument("--stubs", required=True)
  p.add_argument("--spec-map", required=True,
                 help="file of rel_path=exec_path: one compile spec per source")
  p.add_argument("--dep-dir", action="append", default=[],
                 help="exec_path=name of a dependency install tree")
  p.add_argument("--cache-entry", action="append", default=[])
  p.add_argument("--out-link-spec", required=True)
  p.add_argument("--out-gen-spec", required=True)
  p.add_argument("--out-build-tree", required=True)
  p.set_defaults(func=_cmd_configure)

  p = sub.add_parser("compile")
  p.add_argument("--spec", required=True)
  p.add_argument("--src", required=True, help="exec_path=rel_path")
  p.add_argument("--headers",
                 help="file of exec_path=rel_path of headers to stage")
  p.add_argument("--bundle", help="header bundle written by `scan`")
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--gen-tar")
  p.add_argument("--out-tar", required=True)
  p.set_defaults(func=_cmd_compile)

  p = sub.add_parser("scan")
  p.add_argument("--sources", required=True,
                 help="file of rel_path=src=spec=bundle, one per source")
  p.add_argument("--headers", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--bundle-dep", action="append", default=[],
                 help="name of a dependency whose headers go into bundles")
  p.add_argument("--gen-tar")
  p.set_defaults(func=_cmd_scan)

  p = sub.add_parser("codegen")
  p.add_argument("--spec", required=True)
  p.add_argument("--src-map", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-headers-tar", required=True)
  p.add_argument("--out-sources-tar", required=True)
  p.set_defaults(func=_cmd_codegen)

  p = sub.add_parser("compile_generated")
  p.add_argument("--spec", required=True)
  p.add_argument("--gen-headers-tar", required=True)
  p.add_argument("--gen-sources-tar", required=True)
  p.add_argument("--headers", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-tar", required=True)
  p.set_defaults(func=_cmd_compile_generated)

  p = sub.add_parser("link")
  p.add_argument("--spec", required=True)
  p.add_argument("--cmake-src", action="append", default=[])
  p.add_argument("--obj-tars", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--executable", action="append", default=[],
                 help="build_rel_path=exec_path of an executable to export")
  p.add_argument("--out-tar", required=True)
  p.set_defaults(func=_cmd_link)

  p = sub.add_parser("package")
  p.add_argument("--cmake", required=True)
  p.add_argument("--build-tree", required=True)
  p.add_argument("--libs-tar", required=True)
  p.add_argument("--src-map", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-include", required=True)
  p.add_argument("--out-rest", required=True)
  p.set_defaults(func=_cmd_package)

  args = parser.parse_args(argv)
  args.func(args)


if __name__ == "__main__":
  main(sys.argv[1:])
