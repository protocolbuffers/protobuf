#! /usr/bin/env python
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from distutils import extension
import errno
import os
import setuptools
import shutil
import protoc_lib_deps

def GetVersion():
  """Gets the version from google/protobuf/__init__.py

  Do not import google.protobuf.__init__ directly, because an installed
  protobuf library may be loaded instead."""

  with open(os.path.join('..', 'google', 'protobuf', '__init__.py')) as version_file:
    exec(version_file.read(), globals())
    return __version__

def package_data():
  proto_files = []
  for proto_file in protoc_lib_deps.PROTO_FILES:
    source = os.path.join('../../src', proto_file)
    target = os.path.join('google/protoc', proto_file)
    try:
      os.makedirs(os.path.dirname(target))
    except OSError as error:
      if error.errno == errno.EEXIST:
        pass
      else:
        raise
    shutil.copy(source, target)
    proto_files.append(proto_file)
  return {'google.protoc': proto_files}

def protoc_ext_module():
  plugin_sources = [
      'google/protoc/main.cc'] + [
      os.path.join('../../src', cc_file)
      for cc_file in protoc_lib_deps.CC_FILES]
  plugin_ext = extension.Extension(
      name='google.protoc.protoc_compiler',
      sources=['google/protoc/protoc_compiler.pyx'] + plugin_sources,
      include_dirs=[
          '.',
          '../../src',
      ],
      language='c++',
      define_macros=[('HAVE_PTHREAD', 1)],
  )
  return plugin_ext

def maybe_cythonize(exts):
  from Cython import Build
  return Build.cythonize(exts)

setuptools.setup(
  name='protoc',
  version=GetVersion(),
  description='Protocol Buffers Code Generator',
  url='https://developers.google.com/protocol-buffers/',
  maintainer='protobuf@googlegroups.com',
  maintainer_email='protobuf@googlegroups.com',
  license='New BSD License',
  ext_modules=maybe_cythonize([
      protoc_ext_module(),
  ]),
  packages=setuptools.find_packages('.'),
  namespace_packages=['google'],
  package_data=package_data(),
)
