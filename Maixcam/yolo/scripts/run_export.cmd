@echo off
setlocal
cd /d "%~dp0"

set "PYTHON=%~dp0.export-venv\Scripts\python.exe"
if not exist "%PYTHON%" (
    echo Export environment not found: %PYTHON%
    pause
    exit /b 1
)

"%PYTHON%" "%~dp0export_onnx.py" %*
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" (
    echo Export failed with exit code %EXIT_CODE%.
    pause
)
exit /b %EXIT_CODE%

