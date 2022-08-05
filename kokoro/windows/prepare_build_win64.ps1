$ErrorActionPreference = 'Stop'
Set-PSDebug -Trace 1

# Update Chocolatey
#'choco' upgrade -y --no-progress chocolatey

# Enable long paths.
Set-Itemproperty -path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name 'LongPathsEnabled' -value '1'

# Select Visual Studio 2017.
cmd.exe /c "\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

# Update git submodules.
Invoke-Expression -Command 'git submodule update --init --recursive'