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
  compile    Runs the recorded compile command(s) for one source file.
  link       Replays the recorded archive/link commands over all objects.
  package    Runs `cmake --install` into a declared output directory.

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
import json
import os
import re
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
  """Links each `exec_path=name` dependency install tree at <work>/deps/name.

  Returns the staged prefixes, for CMAKE_PREFIX_PATH.
  """
  prefixes = []
  for mapping in mappings:
    exec_path, name = mapping.split("=", 1)
    dst = os.path.join(work, "deps", name)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    os.symlink(os.path.join(execroot, exec_path), dst)
    prefixes.append(dst)
  return prefixes


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
  unowned = []
  for entry in compdb:
    src = os.path.normpath(os.path.join(entry["directory"], entry["file"]))
    command = entry["command"]
    compile_cmd_set.add(command)
    if not src.startswith(src_dir + os.sep):
      unowned.append(src)
      continue
    rel = os.path.relpath(src, src_dir)
    compile_cmds.setdefault(rel, []).append(
        [entry["output"], _relativize(command, work, execroot)])

  if unowned:
    _log("CMake compiles files outside the source tree (e.g. generated "
         "sources), which cmake_bazel does not support yet:\n  " +
         "\n  ".join(sorted(unowned)))
    sys.exit(1)

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
  for command in subprocess.check_output(
      [ninja, "-C", build_dir, "-t", "commands"], text=True).splitlines():
    if command and command not in compile_cmd_set:
      if " @" in command:
        _log("response files are not supported: " + command)
        sys.exit(1)
      link_cmds.append(_relativize(command, work, execroot))

  # Directories the build writes into. Ninja creates an output's directory
  # before running its command; the replayed commands do not.
  output_dirs = set()
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


def _cmd_compile(args):
  execroot = os.getcwd()
  work = tempfile.mkdtemp(prefix="cmake_bazel_cc_")
  src_dir = os.path.join(work, "src")
  build_dir = os.path.join(work, "build")
  os.makedirs(build_dir)
  _stage_files(execroot, src_dir, [args.src] + _read_args_file(args.headers))
  _stage_deps(execroot, work, args.dep_dir)

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

  out_dir = os.path.join(execroot, args.out_dir)
  os.makedirs(out_dir, exist_ok=True)
  _run([_abs_tool(args.cmake, execroot), "--install", build_dir, "--prefix",
        out_dir], cwd=work)
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
  p.add_argument("--out-build-tree", required=True)
  p.set_defaults(func=_cmd_configure)

  p = sub.add_parser("compile")
  p.add_argument("--spec", required=True)
  p.add_argument("--src", required=True, help="exec_path=rel_path")
  p.add_argument("--headers", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-tar", required=True)
  p.set_defaults(func=_cmd_compile)

  p = sub.add_parser("link")
  p.add_argument("--spec", required=True)
  p.add_argument("--cmake-src", action="append", default=[])
  p.add_argument("--obj-tars", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-tar", required=True)
  p.set_defaults(func=_cmd_link)

  p = sub.add_parser("package")
  p.add_argument("--cmake", required=True)
  p.add_argument("--build-tree", required=True)
  p.add_argument("--libs-tar", required=True)
  p.add_argument("--src-map", required=True)
  p.add_argument("--dep-dir", action="append", default=[])
  p.add_argument("--out-dir", required=True)
  p.set_defaults(func=_cmd_package)

  args = parser.parse_args(argv)
  args.func(args)


if __name__ == "__main__":
  main(sys.argv[1:])
