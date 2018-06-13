setlocal

IF %platform%==MinGW GOTO build_mingw
IF %language%==cpp GOTO build_cpp
IF %language%==csharp GOTO build_csharp
IF %language%==python GOTO build_python

echo Unsupported language %language% and platform %platform%. Exiting.
goto :error

:build_mingw
echo Building MinGW
set PATH=C:\mingw-w64\x86_64-7.2.0-posix-seh-rt_v5-rev1\mingw64\bin;%PATH:C:\Program Files\Git\usr\bin;=%
mkdir build_mingw
cd build_mingw
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% -Dprotobuf_BUILD_TESTS=0 ../cmake
mingw32-make -j8 all || goto error
rem cd %configuration%
rem tests.exe || goto error
goto :EOF

:build_cpp
echo Building C++
mkdir build_msvc
cd build_msvc
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% ../cmake
msbuild protobuf.sln /p:Platform=%vcplatform% /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll" || goto error
cd %configuration%
tests.exe || goto error
goto :EOF

:build_csharp
echo Building C#
cd csharp\src
REM The platform environment variable is implicitly used by msbuild;
REM we don't want it.
set platform=
dotnet restore
dotnet build -c %configuration% || goto error

echo Testing C#
dotnet test -c %configuration% -f netcoreapp1.0 Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error
dotnet test -c %configuration% -f net451 Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error

goto :EOF

:build_python
echo Building Python 2.7
c:\python27\python.exe --version
c:\python27\python.exe -m pip --version
c:\python27\python.exe -m pip install flake8
echo stop the build if there are Python syntax errors or undefined names
c:\python27\python.exe -m flake8 . --count --select=E901,E999,F821,F822,F823 --show-source --statistics
REM c:\python27\python.exe -m flake8 . --count --select=E901,E999,F821,F822,F823 --show-source --statistics || goto error
echo exit-zero treats all errors as warnings.  The GitHub editor is 127 chars wide
c:\python27\python.exe -m flake8 . --count --exit-zero --ignore=E111,E114 --max-complexity=10 --max-line-length=127 --statistics

echo Building Python 3.6
c:\python36\python.exe --version
c:\python36\python.exe -m pip --version
c:\python36\python.exe -m pip install flake8
echo stop the build if there are Python syntax errors or undefined names
c:\python36\python.exe -m flake8 . --count --select=E901,E999,F821,F822,F823 --show-source --statistics
REM c:\python36\python.exe -m flake8 . --count --select=E901,E999,F821,F822,F823 --show-source --statistics || goto error
echo exit-zero treats all errors as warnings.  The GitHub editor is 127 chars wide
c:\python36\python.exe -m flake8 . --count --exit-zero --ignore=E111,E114 --max-complexity=10 --max-line-length=127 --statistics
goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
