#!/bin/sh
APP="/home/pi/raspberry_pi_gateway"
LOG="$APP/gateway.log"

if ! ss -ltn | grep -q ':8000 '; then
  cd "$APP" || exit 1
  PYTHONPATH="$APP/src" WARDROBE_HOST=0.0.0.0 WARDROBE_PORT=8000 \
    WARDROBE_CAMERA_COMMAND='v4l2-ctl -d /dev/video{index} --set-ctrl=auto_exposure=3,brightness=0,gain=0,white_balance_automatic=1,power_line_frequency=1 >/dev/null 2>&1 || true; ffmpeg -hide_banner -loglevel error -y -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video{index} -frames:v 30 -update 1 {path}' \
    "$APP/.venv/bin/python" "$APP/run.py" >> "$LOG" 2>&1 &
fi

i=0
while [ "$i" -lt 30 ]; do
  if curl -fsS http://127.0.0.1:8000/api/status >/dev/null 2>&1; then
    break
  fi
  i=$((i + 1))
  sleep 1
done

pkill -x chromium 2>/dev/null || true
pkill -x chrome_crashpad_handler 2>/dev/null || true

export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/1000}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export DBUS_SESSION_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS:-unix:path=/run/user/1000/bus}"

exec /usr/bin/chromium \
  --ozone-platform=wayland \
  --password-store=basic \
  --noerrdialogs \
  --disable-infobars \
  --disable-session-crashed-bubble \
  --disable-restore-session-state \
  --no-first-run \
  --start-maximized \
  http://127.0.0.1:8000/cloud
