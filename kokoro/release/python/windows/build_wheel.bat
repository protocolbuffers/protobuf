if %PYTHON%==C:\Python35 set generator=Visual Studio 14
if %PYTHON%==C:\Python35 set vcplatform=Win32

if %PYTHON%==C:\Python35-x64 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\Python35-x64 set vcplatform=x64

if %PYTHON%==C:\Python36 set generator=Visual Studio 14
if %PYTHON%==C:\Python36 set vcplatform=Win32

if %PYTHON%==C:\Python36-x64 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\Python36-x64 set vcplatform=x64

CALL build_python_env.bat

rmdir /s/q protobuf
git clone https://github.com/google/protobuf.git

CALL build.bat
