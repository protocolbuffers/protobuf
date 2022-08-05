@rem enter repo root
cd /d %~dp0\..\..\..

@rem TODO(b/241475022) Use docker to guarantee better stability.

bazel build //src/... || goto :error

bazel test //src/... || goto :error

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
