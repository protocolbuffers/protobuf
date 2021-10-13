#!/usr/bin/env powershell
# Install Python 3.8 for x64 and x86 in order to build wheels on Windows.
# Originally from grpc/tools/internal_ci/helper_scripts/install_python_interpreters.ps1

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'

trap {
    $ErrorActionPreference = "Continue"
    Write-Error $_
    exit 1
}

# Avoid "Could not create SSL/TLS secure channel"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Install-Python {
    Param(
        [string]$PythonVersion,
        [string]$PythonInstaller,
        [string]$PythonInstallPath,
        [string]$PythonInstallerHash
    )
    $PythonInstallerUrl = "https://www.python.org/ftp/python/$PythonVersion/$PythonInstaller.exe"
    $PythonInstallerPath = "C:\tools\$PythonInstaller.exe"

    # Downloads installer
    Write-Host "Downloading the Python installer: $PythonInstallerUrl => $PythonInstallerPath"
    Invoke-WebRequest -Uri $PythonInstallerUrl -OutFile $PythonInstallerPath

    # Validates checksum
    $HashFromDownload = Get-FileHash -Path $PythonInstallerPath -Algorithm MD5
    if ($HashFromDownload.Hash -ne $PythonInstallerHash) {
        throw "Invalid Python installer: failed checksum!"
    }
    Write-Host "Python installer $PythonInstallerPath validated."

    # Installs Python
    & $PythonInstallerPath /passive InstallAllUsers=1 PrependPath=1 Include_test=0 TargetDir=$PythonInstallPath
    if (-Not $?) {
        throw "The Python installation exited with error!"
    }

    # NOTE(lidiz) Even if the install command finishes in the script, that
    # doesn't mean the Python installation is finished. If using "ps" to check
    # for running processes, you might see ongoing installers at this point.
    # So, we needs this "hack" to reliably validate that the Python binary is
    # functioning properly.

    # Wait for the installer process
    Wait-Process -Name $PythonInstaller -Timeout 300
    Write-Host "Installation process exits normally."

    # Validate Python binary
    $PythonBinary = "$PythonInstallPath\python.exe"
    & $PythonBinary -c 'print(42)'
    Write-Host "Python binary works properly."

    # Installs pip
    & $PythonBinary -m ensurepip --user

    Write-Host "Python $PythonVersion installed by $PythonInstaller at $PythonInstallPath."
}

# Python 3.8
$Python38x86Config = @{
    PythonVersion = "3.8.0"
    PythonInstaller = "python-3.8.0"
    PythonInstallPath = "C:\python38_32bit"
    PythonInstallerHash = "412a649d36626d33b8ca5593cf18318c"
}
Install-Python @Python38x86Config

$Python38x64Config = @{
    PythonVersion = "3.8.0"
    PythonInstaller = "python-3.8.0-amd64"
    PythonInstallPath = "C:\python38"
    PythonInstallerHash = "29ea87f24c32f5e924b7d63f8a08ee8d"
}
Install-Python @Python38x64Config

# Python 3.9
$Python39x86Config = @{
    PythonVersion = "3.9.0"
    PythonInstaller = "python-3.9.0"
    PythonInstallPath = "C:\python39_32bit"
    PythonInstallerHash = "4a2812db8ab9f2e522c96c7728cfcccb"
}
Install-Python @Python39x86Config

$Python39x64Config = @{
    PythonVersion = "3.9.0"
    PythonInstaller = "python-3.9.0-amd64"
    PythonInstallPath = "C:\python39"
    PythonInstallerHash = "b61a33dc28f13b561452f3089c87eb63"
}
Install-Python @Python39x64Config
