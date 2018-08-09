REM Prepend newly installed Python to the PATH of this build (this cannot be
REM done from inside the powershell script as it would require to restart
REM the parent CMD process).
SET PATH=%PYTHON%;%PYTHON%\Scripts;%OLD_PATH%
python -m pip install -U pip
pip install wheel

REM Check that we have the expected version and architecture for Python
python --version
python -c "import struct; print(struct.calcsize('P') * 8)"
