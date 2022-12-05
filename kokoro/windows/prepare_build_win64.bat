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