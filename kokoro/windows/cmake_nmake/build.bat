@rem enter repo root
cd /d %~dp0\..\..\..

@rem TODO(b/241475022) Use docker to guarantee better stability.
@rem TODO(b/241484899) Run conformance tests in windows.

@rem Update git submodules
git submodule update --init --recursive

md build -ea 0
md %KOKORO_ARTIFACTS_DIR%\logs -ea 0

cd build

cmake .. ^
	-G "NMake Makefiles" ^
	-Dprotobuf_BUILD_CONFORMANCE=ON ^
	-Dprotobuf_WITH_ZLIB=OFF ^
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
