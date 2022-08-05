@rem enter repo root
cd /d %~dp0\..\..\..

@rem TODO(b/241475022) Use docker to guarantee better stability.
@rem TODO(b/241484899) Run conformance tests in windows.

@rem Update git submodules
git submodule update --init --recursive

md build -ea 0
md %KOKORO_ARTIFACTS_DIR%\logs -ea 0

cd build

@rem First install protobuf from source.
cmake .. ^
	-G "Visual Studio 15 2017" ^
	-Dprotobuf_BUILD_CONFORMANCE=OFF ^
	-Dprotobuf_WITH_ZLIB=OFF || goto :error

cmake --build . || goto :error

cmake --install . || goto :error

@rem Next run tests forcing the use of our installation.

rm -rf *

cmake .. ^
	-G "Visual Studio 15 2017" ^
	-Dprotobuf_REMOVE_INSTALLED_HEADERS=ON ^
  	-Dprotobuf_BUILD_PROTOBUF_BINARIES=OFF ^
	-Dprotobuf_BUILD_CONFORMANCE=OFF ^
	-Dprotobuf_TEST_XML_OUTDIR=%KOKORO_ARTIFACTS_DIR%\logs\ || goto :error

cmake --build . || goto :error

ctest --verbose -C Debug || goto :error

goto :success

:error
cd ..
echo Failed!
exit /b 1

:success
cd ..
