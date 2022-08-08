@rem enter repo root
cd /d %~dp0\..\..\..

call kokoro\windows\prepare_build_win64.bat || goto :error

@rem Allow Bazel to create short paths.
fsutil 8dot3name set 0

@rem TODO(b/241475022) Use docker to guarantee better stability.

@rem Reinstall Bazel due to corupt installation in kokoro.
bazel version
choco install bazel -y -i
bazel version

@rem Make paths as short as possible to avoid long path issues.
set BAZEL_STARTUP=--output_user_root=C:/tmp

@rem Build libraries first.
bazel %BAZEL_STARTUP% build //:protoc //:protobuf //:protobuf_lite -k || goto :error

@rem Run C++ tests.
bazel %BAZEL_STARTUP% test //src/... -k --test_output=streamed || goto :error

goto :EOF

:error
echo Failed!
exit /b 1
