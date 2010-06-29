@echo off
rem Run a program under a particular version of the .Net framework
rem by setting the COMPLUS_Version environment variable.
rem
rem This command was written by Charlie Poole for the NUnit project.
rem You may use it separately from NUnit at your own risk.

if "%1"=="/?" goto help
if "%1"=="?" goto help
if "%1"=="" goto GetVersion
if /I "%1"=="off" goto RemoveVersion
if "%2"=="" goto SetVersion
goto main

:help
echo Control the version of the .Net framework that is used. The
echo command has several forms:
echo.
echo CLR
echo   Reports the version of the CLR that has been set
echo.
echo CLR version
echo   Sets the local shell environment to use a specific
echo   version of the CLR for subsequent commands.
echo.
echo CLR version command [arguments]
echo   Executes a single command using the specified CLR version.
echo.
echo CLR off
echo   Turns off specific version selection for commands
echo.
echo The CLR version may be specified as vn.n.n or n.n.n. In addition,
echo the following shortcuts are recognized:
echo   net-1.0, 1.0           For version 1.0.3705
echo   net-1.1, 1.1           For version 1.1.4322
echo   beta2                  For version 2.0.50215
echo   net-2.0, 2.0           For version 2.0.50727
echo.
echo NOTE:
echo   Any specific settings for required or supported runtime in 
echo   the ^<startup^> section of a program's config file will 
echo   override the version specified by this command, and the
echo   command will have no effect.
echo.
goto done

:main

setlocal
set CMD=
call :SetVersion %1
shift /1

:loop 'Copy remaining arguments to form the command
if "%1"=="" goto run
set CMD=%CMD% %1
shift /1
goto :loop

:run 'Execute the command
%CMD%
endlocal
goto done

:SetVersion
set COMPLUS_Version=%1

rem Substitute proper format for certain names
if /I "%COMPLUS_Version:~0,1%"=="v"    goto useit
if /I "%COMPLUS_Version%"=="net-1.0"   set COMPLUS_Version=v1.0.3705&goto report
if /I "%COMPLUS_Version%"=="1.0"       set COMPLUS_Version=v1.0.3705&goto report
if /I "%COMPLUS_Version%"=="net-1.1"   set COMPLUS_Version=v1.1.4322&goto report
if /I "%COMPLUS_Version%"=="1.1"       set COMPLUS_Version=v1.1.4322&goto report
if /I "%COMPLUS_Version%"=="beta2"     set COMPLUS_Version=v2.0.50215&goto report
if /I "%COMPLUS_Version%"=="net-2.0"   set COMPLUS_Version=v2.0.50727&goto report
if /I "%COMPLUS_Version%"=="2.0"       set COMPLUS_Version=v2.0.50727&goto report

rem Add additional substitutions here, branching to report

rem assume it's a version number without 'v'
set COMPLUS_Version=v%COMPLUS_Version% 

:report
echo Setting CLR version to %COMPLUS_Version%
goto done

:GetVersion
if "%COMPLUS_Version%"=="" echo CLR version is not set
if NOT "%COMPLUS_Version%"=="" echo CLR version is set to %COMPLUS_Version%
goto done

:RemoveVersion
set COMPLUS_Version=
echo CLR version is no longer set

:done