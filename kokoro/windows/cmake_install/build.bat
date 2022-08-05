@rem enter repo root
cd /d %~dp0\..\..\..

@rem TODO(b/241475022) Use docker to guarantee better stability.
@rem TODO(b/241484899) Run conformance tests in windows.

@rem Update git submodules
git submodule update --init --recursive

md build -ea 0
md %KOKORO_ARTIFACTS_DIR%\logs -ea 0

@rem First install protobuf from source.
cmake -S . -B build ^
	-G "Visual Studio 16 2019" ^
	-Dprotobuf_BUILD_CONFORMANCE=OFF ^
	-Dprotobuf_WITH_ZLIB=OFF || goto :error

cmake --build build || goto :error

cmake --install build || goto :error

@rem Next run tests forcing the use of our installation.

rm -rf build

cmake -S . -B build ^
	-G "Visual Studio 16 2019" ^
	-Dprotobuf_REMOVE_INSTALLED_HEADERS=ON ^
  	-Dprotobuf_BUILD_PROTOBUF_BINARIES=OFF ^
	-Dprotobuf_BUILD_CONFORMANCE=OFF ^
	-Dprotobuf_TEST_XML_OUTDIR=%KOKORO_ARTIFACTS_DIR%\logs\ || goto :error

cmake --build build || goto :error

cd build
ctest --verbose -C Debug || goto :error
cd ..

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
