@rem Update Chocolatey
choco upgrade -y --no-progress chocolatey
choco install -y --no-progress --pre cmake

@rem Enable long paths.
Powershell.exe -Command "Set-Itemproperty -path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name 'LongPathsEnabled' -value '1'"

@rem Allow Bazel to create short paths.
fsutil 8dot3name set 0

@rem Select Visual Studio 2017.
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

@rem Update git submodules.
git submodule update --init --recursive