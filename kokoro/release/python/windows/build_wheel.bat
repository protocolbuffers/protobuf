if %PYTHON%==C:\python35_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python35_32bit set vcplatform=Win32

if %PYTHON%==C:\python35 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python35 set vcplatform=x64

if %PYTHON%==C:\python36_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python36_32bit set vcplatform=Win32

if %PYTHON%==C:\python36 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python36 set vcplatform=x64

CALL build_python_env.bat

rmdir /s/q protobuf
git clone https://github.com/google/protobuf.git

CALL build.bat
