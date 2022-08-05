$ErrorActionPreference = 'Stop'
Set-PSDebug -Trace 1

# Update Chocolatey
'choco' upgrade -y --no-progress chocolatey
'choco' install -y --no-progress --pre cmake

# Enable long paths.
Set-Itemproperty -path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name 'LongPathsEnabled' -value '1'

# Allow Bazel to create short paths.
fsutil 8dot3name set 0

# Select Visual Studio 2017.
cmd.exe /c "\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

# Update git submodules.
Invoke-Expression -Command 'git submodule update --init --recursive'