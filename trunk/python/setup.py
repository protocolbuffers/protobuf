#! /usr/bin/python
#
# See README for usage instructions.

# We must use setuptools, not distutils, because we need to use the
# namespace_packages option for the "google" package.
from ez_setup import use_setuptools
use_setuptools()

from setuptools import setup
from distutils.spawn import find_executable
import sys
import os

maintainer_email = "protobuf@googlegroups.com"

# Find the Protocol Compiler.
if os.path.exists("../src/protoc"):
  protoc = "../src/protoc"
else:
  protoc = find_executable("protoc")

def generate_proto(source):
  """Invokes the Protocol Compiler to generate a _pb2.py from the given
  .proto file.  Does nothing if the output already exists and is newer than
  the input."""

  output = source.replace(".proto", "_pb2.py").replace("../src/", "")

  if not os.path.exists(source):
    print "Can't find required file: " + source
    sys.exit(-1)

  if (not os.path.exists(output) or
      (os.path.exists(source) and
       os.path.getmtime(source) > os.path.getmtime(output))):
    print "Generating %s..." % output

    if protoc == None:
      sys.stderr.write(
          "protoc is not installed nor found in ../src.  Please compile it "
          "or install the binary package.\n")
      sys.exit(-1)

    protoc_command = protoc + " -I../src -I. --python_out=. " + source
    if os.system(protoc_command) != 0:
      sys.exit(-1)

def MakeTestSuite():
  # This is apparently needed on some systems to make sure that the tests
  # work even if a previous version is already installed.
  if 'google' in sys.modules:
    del sys.modules['google']

  generate_proto("../src/google/protobuf/unittest.proto")
  generate_proto("../src/google/protobuf/unittest_import.proto")
  generate_proto("../src/google/protobuf/unittest_mset.proto")
  generate_proto("google/protobuf/internal/more_extensions.proto")
  generate_proto("google/protobuf/internal/more_messages.proto")

  import unittest
  import google.protobuf.internal.generator_test     as generator_test
  import google.protobuf.internal.decoder_test       as decoder_test
  import google.protobuf.internal.descriptor_test    as descriptor_test
  import google.protobuf.internal.encoder_test       as encoder_test
  import google.protobuf.internal.input_stream_test  as input_stream_test
  import google.protobuf.internal.output_stream_test as output_stream_test
  import google.protobuf.internal.reflection_test    as reflection_test
  import google.protobuf.internal.service_reflection_test \
    as service_reflection_test
  import google.protobuf.internal.text_format_test   as text_format_test
  import google.protobuf.internal.wire_format_test   as wire_format_test

  loader = unittest.defaultTestLoader
  suite = unittest.TestSuite()
  for test in [ generator_test,
                decoder_test,
                descriptor_test,
                encoder_test,
                input_stream_test,
                output_stream_test,
                reflection_test,
                service_reflection_test,
                text_format_test,
                wire_format_test ]:
    suite.addTest(loader.loadTestsFromModule(test))

  return suite

if __name__ == '__main__':
  # TODO(kenton):  Integrate this into setuptools somehow?
  if len(sys.argv) >= 2 and sys.argv[1] == "clean":
    # Delete generated _pb2.py files and .pyc files in the code tree.
    for (dirpath, dirnames, filenames) in os.walk("."):
      for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        if filepath.endswith("_pb2.py") or filepath.endswith(".pyc"):
          os.remove(filepath)
  else:
    # Generate necessary .proto file if it doesn't exist.
    # TODO(kenton):  Maybe we should hook this into a distutils command?
    generate_proto("../src/google/protobuf/descriptor.proto")

  setup(name = 'protobuf',
        version = '2.0.1',
        packages = [ 'google' ],
        namespace_packages = [ 'google' ],
        test_suite = 'setup.MakeTestSuite',
        # Must list modules explicitly so that we don't install tests.
        py_modules = [
          'google.protobuf.internal.decoder',
          'google.protobuf.internal.encoder',
          'google.protobuf.internal.input_stream',
          'google.protobuf.internal.message_listener',
          'google.protobuf.internal.output_stream',
          'google.protobuf.internal.type_checkers',
          'google.protobuf.internal.wire_format',
          'google.protobuf.descriptor',
          'google.protobuf.descriptor_pb2',
          'google.protobuf.message',
          'google.protobuf.reflection',
          'google.protobuf.service',
          'google.protobuf.service_reflection',
          'google.protobuf.text_format' ],
        url = 'http://code.google.com/p/protobuf/',
        maintainer = maintainer_email,
        maintainer_email = 'protobuf@googlegroups.com',
        license = 'Apache License, Version 2.0',
        description = 'Protocol Buffers',
        long_description =
          "Protocol Buffers are Google's data interchange format.",
        )
