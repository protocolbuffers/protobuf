@ECHO OFF
IF EXIST "C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\sn.exe" GOTO FOUND
goto USEPATH

:FOUND
"C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\sn.exe" -k %~dp0\Google.ProtocolBuffers.snk 
GOTO EXIT

:USEPATH
sn.exe -k %~dp0\Google.ProtocolBuffers.snk 
GOTO EXIT

:EXIT