@echo off
setlocal
set CAMERA_IP=%~1
if "%CAMERA_IP%"=="" set CAMERA_IP=192.168.137.201
if not exist "%~dp0recordings" mkdir "%~dp0recordings"
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set TS=%%i
set OUTPUT=%~dp0recordings\ball_test_%TS%.mp4
set STREAM=rtsp://%CAMERA_IP%:8554/live
echo Live view: %STREAM%
echo Output: %OUTPUT%
where ffmpeg >nul 2>nul
if errorlevel 1 goto opencv_fallback
where ffplay >nul 2>nul
if errorlevel 1 goto opencv_fallback

start "Ball Live %CAMERA_IP%" ffplay -hide_banner -loglevel warning -rtsp_transport tcp -fflags nobuffer -flags low_delay -framedrop -window_title "Ball Live %CAMERA_IP%" "%STREAM%"
echo Recording started. Press Q in this console to finish and finalize MP4.
ffmpeg -hide_banner -loglevel info -rtsp_transport tcp -i "%STREAM%" -map 0:v:0 -c:v copy -movflags +faststart "%OUTPUT%"
goto finish

:opencv_fallback
set OPENCV_PYTHON=D:\Maixcam\yolo\.venv\Scripts\python.exe
if not exist "%OPENCV_PYTHON%" (
  echo Neither ffmpeg/ffplay nor the prepared OpenCV Python environment was found.
  exit /b 1
)
echo ffmpeg/ffplay not found. Using OpenCV display and MP4 fallback.
"%OPENCV_PYTHON%" "%~dp0view_record_rtsp.py" "%CAMERA_IP%" "%OUTPUT%"

:finish
endlocal
