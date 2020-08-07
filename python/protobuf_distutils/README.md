# Python setuptools extension

This is an extension for Python setuptools which uses an installed protobuf
compiler (`protoc`) to generate Python sources.

## Installing

To use this extension, it needs to be installed so it can be imported by other
projects' setup.py.

```python
$ python setup.py build
$ python setup.py install
```

(If you want to test changes to the extension, you can use `python setup.py
develop`.)

## Usage

### Example setup.py configuration

```python
from setuptools import setup
setup(
    # ...
    name='example_project',

    # Require this package, but only for setup (not installation):
    setup_requires=['protobuf_distutils'],

    options={
        # See below for details.
        'generate_py_protobufs': {
            'source_dir':        'path/to/protos',
            'extra_proto_paths': ['path/to/other/project/protos'],
            'output_dir':        'path/to/project/sources',  # default '.'
            'proto_files':       ['relative/path/to/just_this_file.proto'],
            'protoc':            'path/to/protoc.exe',
        },
    },
)
```

### Example build invocation

These steps will generate protobuf sources so they are included when building
and installing `example_project` (see above):

```shell
$ python setup.py generate_py_protobufs
$ python setup.py build
$ python setup.py install
```

## Options

- `source_dir`:

  Sets the .proto file root path. This will be used to find proto files, and
  to resolve imports.

  The default behavior is to generate sources for all .proto files found
  under `source_dirs`, recursively. This can be controlled with options
  below.

- `extra_proto_paths`:

  Specifies additional paths that should be used to find imports, in
  addition to `source_dir`.

  This option can be used to specify the path to other protobuf sources,
  which are separately converted into Python sources.  No Python code will
  be generated for .proto files under these paths.

- `output_dir`:

  Specifies where generated code should be placed.

  Typically, this should be the project root package. The generated files
  will be placed under `output_dir` according to the relative source paths
  under `source_dir`.

  For example, the source file `${source_dir}/subdir/message.proto`
  will be generated as the Python file `${output_dir}/subdir/message_pb2.py`.

- `proto_files`:

  Specific .proto files can be specified for generating code, instead of
  searching for all .proto files under `source_path`.

  These paths are relative to `source_dir`. For example, to generate code
  for just ${source_dir}/subdir/message.proto:

- `protoc`:

  By default, the protoc binary (the Protobuf compiler) is found by
  searching the environment path. To use a specific protoc binary, its
  path can be specified.
