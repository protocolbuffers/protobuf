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

"""Setuptools/distutils extension for generating Python protobuf code.

This extension uses a prebuilt 'protoc' binary to generate Python types for
protobuf sources. By default, it will use a system-installed protoc binary, but
a custom protoc can be specified by flag.

This command should usually be run before the 'build' command, so that the
generated sources are treated the same way as the rest of the Python
sources.

Options:

    source_dir:
        This is the directory holding .proto files to be processed.

        The default behavior is to generate sources for all .proto files found
        under `source_dir`, recursively. This behavior can be controlled with
        options below.

    proto_root_path:
        This is the root path for resolving imports in source .proto files.

        The default is the shortest prefix of `source_dir` among:
            [source_dir] + self.extra_proto_paths

    extra_proto_paths:
        Specifies additional paths that should be used to find imports, in
        addition to `source_dir`.

        This option can be used to specify the path to other protobuf sources,
        which are imported by files under `source_dir`.  No Python code will be
        generated for .proto files under `extra_proto_paths`.

    output_dir:
        Specifies where generated code should be placed.

        Typically, this should be the root package that generated Python modules
        should be below.

        The generated files will be named according to the relative source paths
        under `proto_root_path`. For example, this source .proto file:
            ${proto_root_path}/subdir/message.proto
        will correspond to this generated Python module:
            ${output_dir}/subdir/message_pb2.py

    proto_files:
        Specific .proto files can be specified for generating code, instead of
        searching for all .proto files under `source_path`.

        These paths are relative to `source_dir`. For example, to generate code
        for just ${source_dir}/subdir/message.proto, specify
        ['subdir/message.proto'].

    protoc:
        By default, the protoc binary (the Protobuf compiler) is found by
        searching the environment path. To use a specific protoc binary, its
        path can be specified.

    recurse:
        If `proto_files` are not specified, then the default behavior is to
        search `source_dir` recursively. This option controls the recursive
        search; if it is False, only .proto files immediately under `source_dir`
        will be used to generate sources.

"""

__author__ = 'dlj@google.com (David L. Jones)'

from setuptools import setup, find_packages

setup(
    name='protobuf_distutils',
    version='1.0',
    packages=find_packages(),
    maintainer='protobuf@googlegroups.com',
    maintainer_email='protobuf@googlegroups.com',
    license='3-Clause BSD License',
    classifiers=[
        "Framework :: Setuptools Plugin",
        "Operating System :: OS Independent",
        # These Python versions should match the protobuf package:
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Topic :: Software Development :: Code Generators",
    ],
    description=('This is a distutils extension to generate Python code for '
                 '.proto files using an installed protoc binary.'),
    url='https://github.com/protocolbuffers/protobuf/',
    entry_points={
        'distutils.commands': [
            ('generate_py_protobufs = '
             'protobuf_distutils.generate_py_protobufs:generate_py_protobufs'),
        ],
    },
)
