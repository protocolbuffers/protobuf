@ECHO OFF
REM ---- Converts the solution to Visual Studio 2010 ----
PUSHD %~dp0
ECHO Microsoft Visual Studio Solution File, Format Version 11.00> Temp.sln
ECHO # Visual Studio 2010>> Temp.sln
type ProtocolBuffers.sln | FIND /V " Visual Studio " >> Temp.sln
move /Y Temp.sln ProtocolBuffers.sln
POPD
