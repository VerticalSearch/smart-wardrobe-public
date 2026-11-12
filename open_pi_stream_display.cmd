@echo off
setlocal

set "ROOT=%~dp0"
set "PYTHON=C:\Users\ECHO\AppData\Local\Programs\Python\Python314\python.exe"
set "SERVER=%ROOT%tools\wardrobe_cloud_display_auto_server.py"
set "DISPLAY_URL=http://127.0.0.1:8767/"
set "WARDROBE_PI_STREAM_URL=http://10.80.7.211:8000/api/camera/stream.mjpg"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$listener = Get-NetTCPConnection -LocalPort 8767 -State Listen -ErrorAction SilentlyContinue; " ^
  "if (-not $listener) { " ^
  "  $env:WARDROBE_PI_STREAM_URL = '%WARDROBE_PI_STREAM_URL%'; " ^
  "  Start-Process -FilePath '%PYTHON%' -ArgumentList @('%SERVER%') -WorkingDirectory '%ROOT%' -WindowStyle Hidden; " ^
  "  Start-Sleep -Seconds 1; " ^
  "} " ^
  "Start-Process '%DISPLAY_URL%';"

endlocal
