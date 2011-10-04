@ECHO OFF
IF "%1" == "2008" GOTO COPY2008
IF "%1" == "2010" GOTO COPY2010
GOTO HELP

:COPY2008
type ProtocolBuffers.sln | FIND " Visual Studio " > Temp.sln
type ProtocolBuffers2008.sln | FIND /V " Visual Studio " >> Temp.sln
move /Y Temp.sln ProtocolBuffers.sln
GOTO EXIT

:COPY2010
type ProtocolBuffers2008.sln | FIND " Visual Studio " > Temp.sln
type ProtocolBuffers.sln | FIND /V " Visual Studio " >> Temp.sln
move /Y Temp.sln ProtocolBuffers2008.sln
GOTO EXIT

:HELP
ECHO.
ECHO Specify the Visiual Studio edition (2008/2010)
ECHO Example:  %~nx0 2008
ECHO.

:EXIT