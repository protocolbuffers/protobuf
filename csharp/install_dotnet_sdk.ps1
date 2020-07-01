#!/usr/bin/env powershell
# Install dotnet SDK using the official dotnet-install.ps1 script

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'

# avoid "Unknown error on a send" in Invoke-WebRequest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$InstallScriptUrl = 'https://dot.net/v1/dotnet-install.ps1'
$InstallScriptPath = Join-Path  "$env:TEMP" 'dotnet-install.ps1'

# Download install script
Write-Host "Downloading install script: $InstallScriptUrl => $InstallScriptPath"
Invoke-WebRequest -Uri $InstallScriptUrl -OutFile $InstallScriptPath

# The SDK versions to install should be kept in sync with versions
# installed by kokoro/linux/dockerfile/test/csharp/Dockerfile
&$InstallScriptPath -Version 2.1.802
&$InstallScriptPath -Version 3.1.301
