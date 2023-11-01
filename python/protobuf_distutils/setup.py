# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Setuptools extension for generating Python protobuf code."""

__author__ = 'dlj@google.com (David L. Jones)'

from os import path
from setuptools import setup, find_packages

# Use README.md as the source for long_description.
this_directory = path.abspath(path.dirname(__file__))
with open(path.join(this_directory, 'README.md'), encoding='utf-8') as f:
    _readme = f.read()

setup(
    name='protobuf_distutils',
    version='1.0',
    packages=find_packages(),
    maintainer='protobuf@googlegroups.com',
    maintainer_email='protobuf@googlegroups.com',
    license='BSD-3-Clause',
    classifiers=[
        'Framework :: Setuptools Plugin',
        'Operating System :: OS Independent',
        # These Python versions should match the protobuf package:
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Topic :: Software Development :: Code Generators',
    ],
    description=(
        'This is a setuptools extension to generate Python code for '
        '.proto files using an installed protoc binary.'
    ),
    long_description=_readme,
    long_description_content_type='text/markdown',
    url='https://github.com/protocolbuffers/protobuf/',
    entry_points={
        'distutils.commands': [
            (
                'generate_py_protobufs = '
                'protobuf_distutils.generate_py_protobufs:generate_py_protobufs'
            ),
        ],
    },
)
