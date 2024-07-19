# Compile the port for QNX

**NOTE**: QNX ports are only supported from a Linux host operating system

- Setup your QNX SDP environment
- From the project root folder, type:

```
git submodule update --init
make -C qnx/build install JLEVEL=4 [INSTALL_ROOT_nto=PATH_TO_YOUR_STAGING_AREA USE_INSTALL_ROOT=true]
```
# Testing

**NOTE**: If you want to execute protobuf tests in a QNX Virtual Target, make sure that it has enough disk space and RAM.

- upload the installed files to the target (keep folder structure unchanged)
- set PATH and LD_LIBRARY_PATH variables
- run **tests** executable

# Known issues

The following test cases fail due to floating point number conversion issues:
```
[  FAILED  ] TextFormatTest.PrintFloatPrecision
[  FAILED  ] TextFormatTest.PrintFieldsInIndexOrder
```