from distutils.core import setup, Extension

setup(name='upb',
      version='0.1',
      ext_modules=[
          Extension('upb.__init__', ['upb.c'],
              include_dirs=['../../'],
              define_macros=[("UPB_UNALIGNED_READS_OK", 1)],
              library_dirs=['../../upb'],
              libraries=['upb_pic'],
          ),
      ],
      packages=['upb']
      )
