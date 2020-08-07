import glob
import sys
import os
import distutils.spawn as spawn
from distutils.cmd import Command
from distutils.errors import DistutilsOptionError, DistutilsExecError

class generate_py_protobufs(Command):
    """Generates Python sources for .proto files."""

    description = 'Generate Python sources for .proto files'
    user_options = [
        ('extra-proto-paths=', None,
         'Additional paths to resolve imports in .proto files.'),

        ('protoc', None,
         'Path to a specific `protoc` command to use.'),
    ]
    boolean_options = ['recurse']

    def initialize_options(self):
        """Sets the defaults for the command options."""
        self.source_dir = None
        self.extra_proto_paths = []
        self.output_dir = '.'
        self.proto_files = None
        self.recurse = True
        self.protoc = None

    def finalize_options(self):
        """Sets the final values for the command options.

        Defaults were set in `initialize_options`, but could have been changed
        by command-line options or by other commands.
        """
        self.ensure_dirname('source_dir')
        self.ensure_string_list('extra_proto_paths')

        if self.output_dir is None:
            self.output_dir = '.'
        self.ensure_dirname('output_dir')

        if self.proto_files is None:
            files = glob.glob(os.path.join(self.source_dir, '*.proto'))
            if self.recurse:
                files.extend(glob.glob(os.path.join(self.source_dir, '**', '*.proto')))
            self.proto_files = [f.partition(self.source_dir + os.path.sep)[-1] for f in files]
            if not self.proto_files:
                raise DistutilsOptionError('no .proto files were found under ' + self.source_dir)
                
        self.ensure_string_list('proto_files')

        if self.protoc is None:
            self.protoc = spawn.find_executable('protoc')

    def run(self):
        # Run protoc. It was already resolved, so don't try to resolve
        # through PATH.
        proto_paths = ['--proto_path=' + self.source_dir]
        for extra in self.extra_proto_paths:
            proto_paths.append('--proto_path=' + extra)
        spawn.spawn(
            [self.protoc,
             '--python_out=' + self.output_dir,
            ] + proto_paths + self.proto_files,
            search_path=0)
