#!/usr/bin/env sh
set -eu

BOOT="/boot/firmware"
[ -d "$BOOT" ] || BOOT="/boot"
LOG="/var/log/wardrobe_gateway_install.log"
APP_DIR="/home/pi/raspberry_pi_gateway"
ZIP="$BOOT/wardrobe_pi_gateway.zip"

{
  echo "=== wardrobe gateway install $(date) ==="
  if [ ! -f "$ZIP" ]; then
    echo "Missing package: $ZIP"
    exit 1
  fi
  if ! command -v unzip >/dev/null 2>&1; then
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install -y unzip
  fi

  rm -rf "$APP_DIR"
  unzip -o "$ZIP" -d /home/pi
  chown -R pi:pi "$APP_DIR"

  bash "$APP_DIR/scripts/install_pi.sh"

  install -o pi -g pi -m 0755 -d /home/pi/.config/labwc
  cat > /home/pi/.config/labwc/autostart <<'EOF'
sh -c 'until python3 -c "import urllib.request; urllib.request.urlopen(\"http://127.0.0.1:8000/api/status\", timeout=1)" >/dev/null 2>&1; do sleep 1; done; exec chromium --kiosk --noerrdialogs --disable-infobars http://127.0.0.1:8000/'
EOF
  chown pi:pi /home/pi/.config/labwc/autostart

  echo "Installed gateway package from $ZIP"
} >> "$LOG" 2>&1
