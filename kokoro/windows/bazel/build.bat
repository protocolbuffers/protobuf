@rem enter repo root
cd /d %~dp0\..\..\..

call kokoro\windows\prepare_build_win64.bat || goto :error

@rem Build libraries first.
bazel %BAZEL_STARTUP% build //:protoc //:protobuf //:protobuf_lite %BAZEL_FLAGS% || goto :error

@rem Run C++ tests.
@rem TODO(b/241484899) Enable conformance tests on windows.
bazel %BAZEL_STARTUP% test %BAZEL_FLAGS% ^
  --test_tag_filters=-conformance --build_tag_filters=-conformance ^
  //src/...  @com_google_protobuf_examples//... || goto :error

goto :EOF

:error
echo Failed!
exit /b 1
