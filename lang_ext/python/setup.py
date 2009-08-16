from distutils.core import setup, Extension
setup(name='upb',
      version='0.1',
      ext_modules=[Extension('upb.definition', ['definition.c'],
                             include_dirs=['../../src'],
                             define_macros=[("UPB_USE_PTHREADS", 1),
                                            ("UPB_UNALIGNED_READS_OK", 1)],
                             library_dirs=['../../src'],
                             libraries=['upb']
      )],
      )
