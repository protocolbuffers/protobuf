#!/usr/bin/env powershell
# Install dotnet SDK based on the SDK version from global.json

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'

# avoid "Unknown error on a send" in Invoke-WebRequest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$InstallScriptUrl = 'https://dot.net/v1/dotnet-install.ps1'
$InstallScriptPath = Join-Path  "$env:TEMP" 'dotnet-install.ps1'
$GlobalJsonPath = Join-Path $PSScriptRoot '..' | Join-Path -ChildPath 'global.json'

# Resolve SDK version from global.json file
$GlobalJson = Get-Content -Raw $GlobalJsonPath | ConvertFrom-Json
$SDKVersion = $GlobalJson.sdk.version

# Download install script
Write-Host "Downloading install script: $InstallScriptUrl => $InstallScriptPath"
Invoke-WebRequest -Uri $InstallScriptUrl -OutFile $InstallScriptPath
&$InstallScriptPath -Version $SDKVersion
