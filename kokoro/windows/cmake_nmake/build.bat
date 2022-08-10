@rem enter repo root
cd /d %~dp0\..\..\..

call kokoro\windows\prepare_build_win64.bat || goto :error

@rem TODO(b/241475022) Use docker to guarantee better stability.
@rem TODO(b/241484899) Run conformance tests in windows.

md build -ea 0
md %KOKORO_ARTIFACTS_DIR%\logs -ea 0

cd build

cmake .. ^
	-G "NMake Makefiles" ^
	-DCMAKE_C_COMPILER=cl.exe ^
	-DCMAKE_CXX_COMPILER=cl.exe ^
	-Dprotobuf_BUILD_CONFORMANCE=OFF ^
	-Dprotobuf_WITH_ZLIB=OFF ^
	-Dprotobuf_TEST_XML_OUTDIR=%KOKORO_ARTIFACTS_DIR%\logs\ || goto :error

cmake --build . || goto :error

ctest --verbose -C Debug || goto :error

goto :success

:error
cd /d %~dp0\..\..\..
echo Failed!
exit /b 1

:success
cd ..
