"""Setuptools extension for generating Python protobuf code.

This extension uses a prebuilt 'protoc' binary to generate Python types for
protobuf sources.

This command should usually be run before the 'build' command, so that the
generated sources are treated the same way as the rest of the Python
sources.

Example invocation:
    $ python setup.py generate_py_protobufs
    $ python setup.py build
    $ python setup.py install

Example configuration in setup.py:

    from setuptools import setup
    setup(
        # ...

        # Require this package, but only for setup (not installation):
        setup_requires=['protobuf_distutils'],

        options={

            # These options control behavior of code generation:
            'generate_py_protobufs': {

                # Set the .proto file root path. This will be used to find proto
                # files, and to resolve imports.
                #
                # The default behavior is to generate sources for all .proto
                # files found under `source_dirs`, recursively. This can be
                # controlled with options below.
                'source_dir': 'path/to/protos',

                # Specify additional paths that should be used to find imports,
                # in addition to `source_dir`.

                # This option can be used to specify the path to other protobuf
                # sources, which are separately converted into Python sources.
                # No Python code will be generated for .proto files under these
                # paths.
                'extra_proto_paths': ['path/to/other/project/protos'],

                # Specify where generated code should be placed.
                #
                # Typically, this should be the project root package. The
                # generated files will be placed under `output_dir` according
                # to the relative source paths under `source_dir`.
                #
                # For example, the source file:
                #     ${source_dir}/subdir/message.proto
                # will be generated as this Python file:
                #     ${output_dir}/subdir/message_pb2.py
                'output_dir': 'path/to/project/sources',  # default '.'

                # Specific .proto files can be specified for generating code,
                # instead of searching for all .proto files under
                # `source_path`.
                #
                # These paths are relative to `source_dir`. For example, to
                # generate code for just ${source_dir}/subdir/message.proto:
                'proto_files': ['subdir/message.proto'],

                # By default, the protoc binary (the Protobuf compiler) is
                # found by searching the environment path. To use a specific
                # protoc binary, its path can be specified.
                'protoc': 'path/to/protoc.exe',
            },
        },
    )

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
