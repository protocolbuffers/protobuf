@echo off
FOR /F "tokens=* USEBACKQ" %%F IN (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationpath`) DO (
    SET VS_INSTALL_PATH=%%F
)
SET VS_VARS_BAT=%VS_INSTALL_PATH%\VC\Auxiliary\Build\vcvars64.bat
CALL "%VS_VARS_BAT%"

mkdir install
mkdir install_debug
mkdir build
cd build
mkdir release
cd release
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../install ../..

nmake

nmake install

cd ..

mkdir debug
cd debug

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../install_debug ../..

nmake

nmake install
