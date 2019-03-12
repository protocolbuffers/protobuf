@rem enter repo root
cd /d %~dp0\..\..\..

cd csharp

@rem Install dotnet SDK
powershell -File install_dotnet_sdk.ps1
set PATH=%LOCALAPPDATA%\Microsoft\dotnet;%PATH%

call buildall.bat
