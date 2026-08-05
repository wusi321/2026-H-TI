@echo off
setlocal EnableExtensions

set "RECORDER_DIR=%~dp0webrtc_recorder"

if not exist "%RECORDER_DIR%\start_web_recorder.bat" (
    echo [ERROR] WebRTC recorder is missing:
    echo %RECORDER_DIR%\start_web_recorder.bat
    exit /b 1
)

call "%RECORDER_DIR%\start_web_recorder.bat"
