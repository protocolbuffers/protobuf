@rem enter repo root
cd /d %~dp0\..\..\..\..

cd csharp

@rem see what dotnet version is available
dotnet --version

@rem TODO(jtattermusch): Kokoro workers currently only have dotnet SDK 2.1.3
@rem so we just overwrite the SDK requirement in global.json as the results
@rem should be fully compatible.
echo { "sdk": { "version": "2.1.3" } } >global.json

call build_packages.bat
