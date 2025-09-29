#! /usr/bin/env python
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
# See README for usage instructions.

import glob
import os
import sys

from setuptools import setup, Extension, find_namespace_packages


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

# If at some point the fasttable decoder is ready for prime time, we could
# enable it here. But even then we'll need to disable it on platforms where
# it will not work (eg. 32-bit, MSVC).
fasttable_decoder_enabled = False

srcs = (
    glob.glob('google/protobuf/*.c')
    + glob.glob('python/*.c')
    + glob.glob('upb/**/*.c', recursive=True)
    + glob.glob('utf8_range/*.c')
)

if not fasttable_decoder_enabled:
  # Our heuristic is to match any file that contains `decode_fast` in the name.
  # If we change the file names of the fast decoder, we will have to update
  # this heuristic.
  srcs = list(filter(lambda src: 'decode_fast' not in src, srcs))

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
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
    ],
    packages=find_namespace_packages(include=['google*']),
    install_requires=[],
    ext_modules=[
        Extension(
            'google._upb._message',
            srcs,
            include_dirs=[current_dir, os.path.join(current_dir, 'utf8_range')],
            language='c',
            extra_link_args=extra_link_args,
        )
    ],
    python_requires='>=3.9',
)
