@rem enter repo root
cd /d %~dp0\..\..\..

@rem Select Visual Studio 2017.
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

@rem TODO(b/241475022) Use docker to guarantee better stability.

@rem Reinstall Bazel due to corupt installation in kokoro.
bazel version
choco install bazel -y -i
bazel version

bazel build //:protoc //:protobuf //:protobuf_lite || goto :error

bazel test //src/... --test_output=streamed || goto :error

goto :EOF

:error
echo Failed!
exit /b 1
