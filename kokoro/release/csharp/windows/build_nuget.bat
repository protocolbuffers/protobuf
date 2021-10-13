@rem enter repo root
cd /d %~dp0\..\..\..\..

cd csharp

@rem Install dotnet SDK
powershell -File install_dotnet_sdk.ps1
set PATH=%LOCALAPPDATA%\Microsoft\dotnet;%PATH%

@rem Disable some unwanted dotnet options
set DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
set DOTNET_CLI_TELEMETRY_OPTOUT=true

@rem Work around https://github.com/dotnet/core/issues/5881
dotnet nuget locals all --clear

call build_packages.bat
