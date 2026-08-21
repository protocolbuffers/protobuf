# Python setuptools extension

This is an extension for Python setuptools which uses an installed protobuf
compiler (`protoc`) to generate Python sources.

## Installing

To use this extension, it needs to be installed so it can be imported by other
projects' setup.py.

```shell
$ python setup.py build
$ python -m pip install .
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
$ python -m pip install .
```

## Options

- `source_dir`:

  This is the directory holding .proto files to be processed.

  The default behavior is to generate sources for all .proto files found under
  `source_dir`, recursively. This behavior can be controlled with options below.

- `proto_root_path`:

  This is the root path for resolving imports in source .proto files.

  The default is the shortest prefix of `source_dir` among `[source_dir] +
  self.extra_proto_paths`.

- `extra_proto_paths`:

  Specifies additional paths that should be used to find imports, in
  addition to `source_dir`.

  This option can be used to specify the path to other protobuf sources,
  which are imported by files under `source_dir`.  No Python code will
  be generated for .proto files under `extra_proto_paths`.

- `output_dir`:

  Specifies where generated code should be placed.

  Typically, this should be the root package that generated Python modules
  should be below.

  The generated files will be placed under `output_dir` according to the
  relative source paths under `proto_root_path`. For example, the source file
  `${proto_root_path}/subdir/message.proto` will be generated as the Python
  module `${output_dir}/subdir/message_pb2.py`.

- `proto_files`:

  A list of strings, specific .proto file paths for generating code, instead of
  searching for all .proto files under `source_path`.

  These paths are relative to `source_dir`. For example, to generate code
  for just `${source_dir}/subdir/message.proto`, specify
  `['subdir/message.proto']`.

- `protoc`:

  By default, the protoc binary (the Protobuf compiler) is found by
  searching the environment path. To use a specific protoc binary, its
  path can be specified. Resolution of the `protoc` value is as follows:
  1. If the `--protoc=VALUE` flag is passed to `generate_py_protobufs`,
     then `VALUE` will be used.
     For example:
     ```shell
     $ python setup.py generate_py_protobufs --protoc=/path/to/protoc
     ```
  2. Otherwise, if a value was set in the `options`, it will be used.
     (See "Example setup.py configuration," above.)
  3. Otherwise, if the `PROTOC` environment variable is set, it will be
     used. For example:
     For example:
     ```shell
     $ PROTOC=/path/to/protoc python setup.py generate_py_protobufs
     ```
  4. Otherwise, `$PATH` will be searched.
