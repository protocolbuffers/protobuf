#! /usr/bin/env python
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
#
# See README for usage instructions.

import glob
import os
import sys
import sysconfig

# We must use setuptools, not distutils, because we need to use the
# namespace_packages option for the "google" package.
from setuptools import setup, Extension, find_packages


def GetVersion():
  """Reads and returns the version from google/protobuf/__init__.py.

  Do not import google.protobuf.__init__ directly, because an installed
  protobuf library may be loaded instead.

  Returns:
      The version.
  """

  with open(os.path.join('google', 'protobuf', '__init__.py')) as version_file:
    file_globals = {}
    exec(version_file.read(), file_globals)  # pylint:disable=exec-used
    return file_globals["__version__"]


current_dir = os.path.dirname(os.path.abspath(__file__))
extra_link_args = []

if sys.platform.startswith('win'):
  extra_link_args = ['-static']

setup(
    name='protobuf',
    version=GetVersion(),
    description='Protocol Buffers',
    download_url='https://github.com/protocolbuffers/protobuf/releases',
    long_description="Protocol Buffers are Google's data interchange format",
    url='https://developers.google.com/protocol-buffers/',
    project_urls={
        'Source': 'https://github.com/protocolbuffers/protobuf',
    },
    maintainer='protobuf@googlegroups.com',
    maintainer_email='protobuf@googlegroups.com',
    license='BSD-3-Clause',
    classifiers=[
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
    ],
    namespace_packages=['google'],
    packages=find_packages(),
    install_requires=[],
    ext_modules=[
        Extension(
            'google._upb._message',
            glob.glob('google/protobuf/*.c')
            + glob.glob('python/*.c')
            + glob.glob('upb/**/*.c', recursive=True)
            + glob.glob('utf8_range/*.c'),
            include_dirs=[current_dir, os.path.join(current_dir, 'utf8_range')],
            language='c',
            extra_link_args=extra_link_args,
        )
    ],
    python_requires='>=3.7',
)
