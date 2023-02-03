set generator32=Visual Studio 15
set generator64=Visual Studio 15 Win64
set vcplatform32=win32
set vcplatform64=x64
set configuration=Release

echo Building protoc
cd github\protobuf

@rem Update Chocolatey
choco upgrade -y --no-progress chocolatey
choco install -y python --version 3.10.0
choco install -y --no-progress --pre cmake

@rem Enable long paths.
Powershell.exe -Command "New-ItemProperty -Path HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force"

@rem Update git submodules.
git submodule update --init --recursive

@rem Select Visual Studio 2017.
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

@rem Convert Windows line breaks to Unix line breaks
@rem This allows text-matching tests to pass
@find . -type f -print0 | xargs -0 d2u

@rem Use python3
C:\python310\python.exe -m venv venv
call venv\Scripts\activate.bat

@rem Allow Bazel to create short paths.
fsutil 8dot3name set 0

@rem Reinstall Bazel due to corrupt installation in kokoro.
bazel version
choco install bazel -y -i --version 5.1.0
bazel version

@rem Regenerate stale files.
bazel test //src:cmake_lists_staleness_test || call bazel-bin\src\cmake_lists_staleness_test.exe --fix
bazel test //src/google/protobuf:well_known_types_staleness_test || call bazel-bin\src\google\protobuf\well_known_types_staleness_test.exe --fix

set ABSL_ROOT_DIR=%cd%\third_party\abseil-cpp

mkdir build32
cd build32
cmake -G "%generator32%" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON ..
msbuild protobuf.sln /p:Platform=%vcplatform32% || goto error
cd ..

mkdir build64
cd build64
cmake -G "%generator64%" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON ..
msbuild protobuf.sln /p:Platform=%vcplatform64% || goto error
cd ..

goto :EOF

:error
echo Failed!
exit /b %ERRORLEVEL%
