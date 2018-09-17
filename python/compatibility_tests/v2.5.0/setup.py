#! /usr/bin/env python
#
import glob
import os
import subprocess
import sys

from setuptools import setup, Extension, find_packages

if sys.version_info[0] == 3:
  # Python 3
  from distutils.command.build_py import build_py_2to3 as _build_py
else:
  # Python 2
  from distutils.command.build_py import build_py as _build_py
from distutils.spawn import find_executable

def generate_proto(source, code_gen):
  """Invokes the Protocol Compiler to generate a _pb2.py from the given
  .proto file."""
  output = source.replace(".proto", "_pb2.py").replace("protos/src/proto/", "").replace("protos/python/", "")

  if not os.path.exists(source):
    sys.stderr.write("Can't find required file: %s\n" % source)
    sys.exit(-1)

  protoc_command = [ code_gen, "-Iprotos/src/proto", "-Iprotos/python", "--python_out=.", source ]
  if subprocess.call(protoc_command) != 0:
    sys.exit(-1)

class build_py(_build_py):
  def run(self):
    # generate .proto file
    protoc_1 = "./protoc_1"
    protoc_2 = "./protoc_2"
    generate_proto("protos/src/proto/google/protobuf/unittest.proto", protoc_2)
    generate_proto("protos/src/proto/google/protobuf/unittest_custom_options.proto", protoc_1)
    generate_proto("protos/src/proto/google/protobuf/unittest_import.proto", protoc_1)
    generate_proto("protos/src/proto/google/protobuf/unittest_import_public.proto", protoc_1)
    generate_proto("protos/src/proto/google/protobuf/unittest_mset.proto", protoc_1)
    generate_proto("protos/src/proto/google/protobuf/unittest_no_generic_services.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/factory_test1.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/factory_test2.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/more_extensions.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/more_extensions_dynamic.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/more_messages.proto", protoc_1)
    generate_proto("protos/python/google/protobuf/internal/test_bad_identifiers.proto", protoc_1)

    # _build_py is an old-style class, so super() doesn't work.
    _build_py.run(self)

if __name__ == '__main__':
  # Keep this list of dependencies in sync with tox.ini.
  install_requires = ['six>=1.9', 'setuptools']
  if sys.version_info <= (2,7):
    install_requires.append('ordereddict')
    install_requires.append('unittest2')

  setup(
      name='protobuf',
      description='Protocol Buffers',
      download_url='https://github.com/protocolbuffers/protobuf/releases',
      long_description="Protocol Buffers are Google's data interchange format",
      url='https://developers.google.com/protocol-buffers/',
      maintainer='protobuf@googlegroups.com',
      maintainer_email='protobuf@googlegroups.com',
      license='3-Clause BSD License',
      classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        ],
      packages=find_packages(
          exclude=[
              'import_test_package',
          ],
      ),
      test_suite='tests.google.protobuf.internal',
      cmdclass={
          'build_py': build_py,
      },
      install_requires=install_requires,
  )
