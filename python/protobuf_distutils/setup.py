"""Setuptools extension for generating Python protobuf code.

This extension uses a prebuilt 'protoc' binary to generate Python types for
protobuf sources.

This command should usually be run before the 'build' command, so that the
generated sources are treated the same way as the rest of the Python
sources.

Options:

    source_dir:
        Sets the .proto file root path. This will be used to find proto files, and
        to resolve imports.

        The default behavior is to generate sources for all .proto files found
        under `source_dirs`, recursively. This can be controlled with options
        below.

    extra_proto_paths:
        Specifies additional paths that should be used to find imports, in
        addition to `source_dir`.

        This option can be used to specify the path to other protobuf sources,
        which are separately converted into Python sources.  No Python code will
        be generated for .proto files under these paths.

    output_dir:
        Specifies where generated code should be placed.

        Typically, this should be the project root package. The generated files
        will be placed under `output_dir` according to the relative source paths
        under `source_dir`.

        For example, the source file:
            ${source_dir}/subdir/message.proto
        will be generated as this Python file:
            ${output_dir}/subdir/message_pb2.py

    proto_files:
        Specific .proto files can be specified for generating code, instead of
        searching for all .proto files under `source_path`.

        These paths are relative to `source_dir`. For example, to generate code
        for just ${source_dir}/subdir/message.proto:

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

from setuptools import setup, find_packages

setup(
    name='protobuf_distutils',
    version='1.0',
    packages=find_packages(),
    maintainer='protobuf@googlegroups.com',
    maintainer_email='protobuf@googlegroups.com',
    license='3-Clause BSD License',
    description='This is a distutils extension to generate Python code for .proto files using an installed protoc binary.',
    url='https://github.com/protocolbuffers/protobuf/',
    entry_points={
        'distutils.commands': [
            'generate_py_protobufs = protobuf_distutils.generate_py_protos:generate_py_protobufs',
        ],
    },
)
