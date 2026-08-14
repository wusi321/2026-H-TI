@echo off
setlocal
cd /d "%~dp0"

set "PYTHON=%~dp0.venv\Scripts\python.exe"
if not exist "%PYTHON%" (
    echo Training environment not found: %PYTHON%
    pause
    exit /b 1
)

"%PYTHON%" "%~dp0train_pose_v2.py" %*
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" (
    echo Training failed with exit code %EXIT_CODE%.
    pause
)
exit /b %EXIT_CODE%
