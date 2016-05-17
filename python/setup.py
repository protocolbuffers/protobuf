#! /usr/bin/env python
#
# See README for usage instructions.
import glob
import os
import subprocess
import sys

# We must use setuptools, not distutils, because we need to use the
# namespace_packages option for the "google" package.
from setuptools import setup, Extension, find_packages

from distutils.command.clean import clean as _clean

if sys.version_info[0] == 3:
  # Python 3
  from distutils.command.build_py import build_py_2to3 as _build_py
else:
  # Python 2
  from distutils.command.build_py import build_py as _build_py
from distutils.spawn import find_executable

# Find the Protocol Compiler.
if 'PROTOC' in os.environ and os.path.exists(os.environ['PROTOC']):
  protoc = os.environ['PROTOC']
elif os.path.exists("../src/protoc"):
  protoc = "../src/protoc"
elif os.path.exists("../src/protoc.exe"):
  protoc = "../src/protoc.exe"
elif os.path.exists("../vsprojects/Debug/protoc.exe"):
  protoc = "../vsprojects/Debug/protoc.exe"
elif os.path.exists("../vsprojects/Release/protoc.exe"):
  protoc = "../vsprojects/Release/protoc.exe"
else:
  protoc = find_executable("protoc")


def GetVersion():
  """Gets the version from google/protobuf/__init__.py

  Do not import google.protobuf.__init__ directly, because an installed
  protobuf library may be loaded instead."""

  with open(os.path.join('google', 'protobuf', '__init__.py')) as version_file:
    exec(version_file.read(), globals())
    return __version__


def generate_proto(source, require = True):
  """Invokes the Protocol Compiler to generate a _pb2.py from the given
  .proto file.  Does nothing if the output already exists and is newer than
  the input."""

  if not require and not os.path.exists(source):
    return

  output = source.replace(".proto", "_pb2.py").replace("../src/", "")

  if (not os.path.exists(output) or
      (os.path.exists(source) and
       os.path.getmtime(source) > os.path.getmtime(output))):
    print("Generating %s..." % output)

    if not os.path.exists(source):
      sys.stderr.write("Can't find required file: %s\n" % source)
      sys.exit(-1)

    if protoc is None:
      sys.stderr.write(
          "protoc is not installed nor found in ../src.  Please compile it "
          "or install the binary package.\n")
      sys.exit(-1)

    protoc_command = [ protoc, "-I../src", "-I.", "--python_out=.", source ]
    if subprocess.call(protoc_command) != 0:
      sys.exit(-1)

def GenerateUnittestProtos():
  generate_proto("../src/google/protobuf/map_unittest.proto", False)
  generate_proto("../src/google/protobuf/unittest_arena.proto", False)
  generate_proto("../src/google/protobuf/unittest_no_arena.proto", False)
  generate_proto("../src/google/protobuf/unittest_no_arena_import.proto", False)
  generate_proto("../src/google/protobuf/unittest.proto", False)
  generate_proto("../src/google/protobuf/unittest_custom_options.proto", False)
  generate_proto("../src/google/protobuf/unittest_import.proto", False)
  generate_proto("../src/google/protobuf/unittest_import_public.proto", False)
  generate_proto("../src/google/protobuf/unittest_mset.proto", False)
  generate_proto("../src/google/protobuf/unittest_mset_wire_format.proto", False)
  generate_proto("../src/google/protobuf/unittest_no_generic_services.proto", False)
  generate_proto("../src/google/protobuf/unittest_proto3_arena.proto", False)
  generate_proto("../src/google/protobuf/util/json_format_proto3.proto", False)
  generate_proto("google/protobuf/internal/any_test.proto", False)
  generate_proto("google/protobuf/internal/descriptor_pool_test1.proto", False)
  generate_proto("google/protobuf/internal/descriptor_pool_test2.proto", False)
  generate_proto("google/protobuf/internal/factory_test1.proto", False)
  generate_proto("google/protobuf/internal/factory_test2.proto", False)
  generate_proto("google/protobuf/internal/import_test_package/inner.proto", False)
  generate_proto("google/protobuf/internal/import_test_package/outer.proto", False)
  generate_proto("google/protobuf/internal/missing_enum_values.proto", False)
  generate_proto("google/protobuf/internal/message_set_extensions.proto", False)
  generate_proto("google/protobuf/internal/more_extensions.proto", False)
  generate_proto("google/protobuf/internal/more_extensions_dynamic.proto", False)
  generate_proto("google/protobuf/internal/more_messages.proto", False)
  generate_proto("google/protobuf/internal/packed_field_test.proto", False)
  generate_proto("google/protobuf/internal/test_bad_identifiers.proto", False)
  generate_proto("google/protobuf/pyext/python.proto", False)


class clean(_clean):
  def run(self):
    # Delete generated files in the code tree.
    for (dirpath, dirnames, filenames) in os.walk("."):
      for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        if filepath.endswith("_pb2.py") or filepath.endswith(".pyc") or \
          filepath.endswith(".so") or filepath.endswith(".o") or \
          filepath.endswith('google/protobuf/compiler/__init__.py') or \
          filepath.endswith('google/protobuf/util/__init__.py'):
          os.remove(filepath)
    # _clean is an old-style class, so super() doesn't work.
    _clean.run(self)

class build_py(_build_py):
  def run(self):
    # Generate necessary .proto file if it doesn't exist.
    generate_proto("../src/google/protobuf/descriptor.proto")
    generate_proto("../src/google/protobuf/compiler/plugin.proto")
    generate_proto("../src/google/protobuf/any.proto")
    generate_proto("../src/google/protobuf/api.proto")
    generate_proto("../src/google/protobuf/duration.proto")
    generate_proto("../src/google/protobuf/empty.proto")
    generate_proto("../src/google/protobuf/field_mask.proto")
    generate_proto("../src/google/protobuf/source_context.proto")
    generate_proto("../src/google/protobuf/struct.proto")
    generate_proto("../src/google/protobuf/timestamp.proto")
    generate_proto("../src/google/protobuf/type.proto")
    generate_proto("../src/google/protobuf/wrappers.proto")
    GenerateUnittestProtos()

    # Make sure google.protobuf/** are valid packages.
    for path in ['', 'internal/', 'compiler/', 'pyext/', 'util/']:
      try:
        open('google/protobuf/%s__init__.py' % path, 'a').close()
      except EnvironmentError:
        pass
    # _build_py is an old-style class, so super() doesn't work.
    _build_py.run(self)

class test_conformance(_build_py):
  target = 'test_python'
  def run(self):
    if sys.version_info >= (2, 7):
      # Python 2.6 dodges these extra failures.
      os.environ["CONFORMANCE_PYTHON_EXTRA_FAILURES"] = (
          "--failure_list failure_list_python-post26.txt")
    cmd = 'cd ../conformance && make %s' % (test_conformance.target)
    status = subprocess.check_call(cmd, shell=True)


def get_option_from_sys_argv(option_str):
  if option_str in sys.argv:
    sys.argv.remove(option_str)
    return True
  return False


if __name__ == '__main__':
  ext_module_list = []
  warnings_as_errors = '--warnings_as_errors'
  if get_option_from_sys_argv('--cpp_implementation'):
    # Link libprotobuf.a and libprotobuf-lite.a statically with the
    # extension. Note that those libraries have to be compiled with
    # -fPIC for this to work.
    compile_static_ext = get_option_from_sys_argv('--compile_static_extension')
    extra_compile_args = ['-Wno-write-strings',
                          '-Wno-invalid-offsetof',
                          '-Wno-sign-compare']
    libraries = ['protobuf']
    extra_objects = None
    if compile_static_ext:
      libraries = None
      extra_objects = ['../src/.libs/libprotobuf.a',
                       '../src/.libs/libprotobuf-lite.a']
    test_conformance.target = 'test_python_cpp'

    if "clang" in os.popen('$CC --version 2> /dev/null').read():
      extra_compile_args.append('-Wno-shorten-64-to-32')

    if warnings_as_errors in sys.argv:
      extra_compile_args.append('-Werror')
      sys.argv.remove(warnings_as_errors)

    # C++ implementation extension
    ext_module_list.extend([
        Extension(
            "google.protobuf.pyext._message",
            glob.glob('google/protobuf/pyext/*.cc'),
            include_dirs=[".", "../src"],
            libraries=libraries,
            extra_objects=extra_objects,
            library_dirs=['../src/.libs'],
            extra_compile_args=extra_compile_args,
        ),
        Extension(
            "google.protobuf.internal._api_implementation",
            glob.glob('google/protobuf/internal/api_implementation.cc'),
            extra_compile_args=['-DPYTHON_PROTO2_CPP_IMPL_V2'],
        ),
    ])
    os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'

  # Keep this list of dependencies in sync with tox.ini.
  install_requires = ['six>=1.9', 'setuptools']
  if sys.version_info <= (2,7):
    install_requires.append('ordereddict')
    install_requires.append('unittest2')

  setup(
      name='protobuf',
      version=GetVersion(),
      description='Protocol Buffers',
      download_url='https://github.com/google/protobuf/releases',
      long_description="Protocol Buffers are Google's data interchange format",
      url='https://developers.google.com/protocol-buffers/',
      maintainer='protobuf@googlegroups.com',
      maintainer_email='protobuf@googlegroups.com',
      license='New BSD License',
      classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        ],
      namespace_packages=['google'],
      packages=find_packages(
          exclude=[
              'import_test_package',
          ],
      ),
      test_suite='google.protobuf.internal',
      cmdclass={
          'clean': clean,
          'build_py': build_py,
          'test_conformance': test_conformance,
      },
      install_requires=install_requires,
      ext_modules=ext_module_list,
  )
