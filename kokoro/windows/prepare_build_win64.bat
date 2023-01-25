@rem Update Chocolatey
choco upgrade -y --no-progress chocolatey
choco install -y python --version 3.10.0
choco install -y --no-progress --pre cmake

@rem Enable long paths.
Powershell.exe -Command "New-ItemProperty -Path HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force"

@rem Update git submodules.
git submodule update --init --recursive

@rem Select Visual Studio 2017.
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

@rem Convert Windows line breaks to Unix line breaks
@rem This allows text-matching tests to pass
@find . -type f -print0 | xargs -0 d2u

@rem Use python3
C:\python310\python.exe -m venv venv
call venv\Scripts\activate.bat

@rem Setup Bazel

@rem TODO(b/241475022) Use docker to guarantee better stability.
@rem Allow Bazel to create short paths.
fsutil 8dot3name set 0

@rem Reinstall Bazel due to corrupt installation in kokoro.
bazel version
choco install bazel -y -i --version 5.1.0
bazel version

@rem Set invocation ID so that bazel run is known to kokoro
uuidgen > %KOKORO_ARTIFACTS_DIR%\bazel_invocation_ids
SET /p BAZEL_INTERNAL_INVOCATION_ID=<%KOKORO_ARTIFACTS_DIR%\bazel_invocation_ids

@rem Make paths as short as possible to avoid long path issues.
set BAZEL_STARTUP=--output_user_root=C:/tmp --windows_enable_symlinks
set BAZEL_FLAGS=--enable_runfiles --keep_going --test_output=errors ^
  --verbose_failures ^
  --invocation_id=%BAZEL_INTERNAL_INVOCATION_ID% ^
  --google_credentials=%KOKORO_BAZEL_AUTH_CREDENTIAL%  ^
  --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache/%KOKORO_JOB_NAME%

@rem Regenerate stale CMake configs.
@rem LINT.IfChange(staleness_tests)
bazel test //src:cmake_lists_staleness_test || call bazel-bin\src\cmake_lists_staleness_test.exe --fix
bazel test //src/google/protobuf:well_known_types_staleness_test || call bazel-bin\src\google\protobuf\well_known_types_staleness_test.exe --fix
@rem LINT.ThenChange(//depot/google3/third_party/protobuf/github/regenerate_stale_files.sh)
