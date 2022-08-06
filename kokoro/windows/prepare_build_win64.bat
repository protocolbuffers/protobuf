@rem Update Chocolatey
choco upgrade -y --no-progress chocolatey
choco install -y --no-progress --pre cmake

@rem Enable long paths.
Powershell.exe -Command "New-ItemProperty -Path HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force"

@rem Update git submodules.
git submodule update --init --recursive

@rem Select Visual Studio 2017.
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
