@echo off
setlocal EnableExtensions
title MaixCAM Native WebRTC Recorder

set "APP_DIR=%~dp0"
set "LOG_FILE=%APP_DIR%start_web_recorder.log"
set "NODE_EXE="

pushd "%APP_DIR%" >nul 2>&1
if errorlevel 1 goto :bad_folder

if exist "C:\Program Files\nodejs\node.exe" set "NODE_EXE=C:\Program Files\nodejs\node.exe"
if not defined NODE_EXE if exist "C:\Program Files (x86)\nodejs\node.exe" set "NODE_EXE=C:\Program Files (x86)\nodejs\node.exe"
if not defined NODE_EXE (
    for /f "delims=" %%N in ('where.exe node.exe 2^>nul') do if not defined NODE_EXE set "NODE_EXE=%%N"
)

> "%LOG_FILE%" echo ============================================================
>>"%LOG_FILE%" echo MaixCAM Native WebRTC Recorder startup log
>>"%LOG_FILE%" echo Date: %date% %time%
>>"%LOG_FILE%" echo Folder: %APP_DIR%

echo.
echo MaixCAM Native WebRTC Recorder
echo --------------------------------

if not defined NODE_EXE (
    echo [ERROR] Node.js was not found.
    >>"%LOG_FILE%" echo ERROR: Node.js was not found.
    echo Install Node.js 18 or newer, then run this file again.
    goto :stop
)

if not exist "%APP_DIR%server.js" (
    echo [ERROR] server.js is missing.
    >>"%LOG_FILE%" echo ERROR: server.js is missing.
    goto :stop
)

if not exist "%APP_DIR%device.json" (
    echo [ERROR] device.json is missing.
    >>"%LOG_FILE%" echo ERROR: device.json is missing.
    goto :stop
)

if not exist "%APP_DIR%web\index.html" (
    echo [ERROR] web files are missing.
    >>"%LOG_FILE%" echo ERROR: web files are missing.
    goto :stop
)

echo Node: %NODE_EXE%
echo Starting the local web interface...
echo Keep this window open while using the recorder.
echo.
echo IMPORTANT:
echo   1. Run the MaixCAM native WebRTC program first.
echo   2. Enable the MaixCAM recorder browser extension.
echo.
>>"%LOG_FILE%" echo Node: %NODE_EXE%
>>"%LOG_FILE%" echo Starting server.js

"%NODE_EXE%" "%APP_DIR%server.js" >>"%LOG_FILE%" 2>&1
set "EXIT_CODE=%ERRORLEVEL%"
>>"%LOG_FILE%" echo Server exited with code %EXIT_CODE% at %date% %time%

echo.
echo The web service stopped. Exit code: %EXIT_CODE%
echo Log: %LOG_FILE%
goto :stop

:bad_folder
echo [ERROR] Cannot access the application folder:
echo %APP_DIR%
set "EXIT_CODE=1"

:stop
echo.
echo Press any key to close this window.
pause >nul
popd >nul 2>&1
exit /b %EXIT_CODE%
