@ECHO OFF
SETLOCAL
SET distributionDir=%~dp0
SET beePowerShellScript=%distributionDir%bee.ps1
PowerShell -NoProfile -ExecutionPolicy Bypass -File "%beePowerShellScript%" %*
