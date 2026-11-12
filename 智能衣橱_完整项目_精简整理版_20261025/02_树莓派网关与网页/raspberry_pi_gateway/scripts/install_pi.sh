#!/usr/bin/env bash
set -euo pipefail

APP_DIR="${APP_DIR:-/home/pi/raspberry_pi_gateway}"
SERVICE_NAME="${SERVICE_NAME:-wardrobe-gateway.service}"
USER_NAME="${USER_NAME:-pi}"
if [ "$(id -u)" = "0" ]; then
  SUDO=""
else
  SUDO="sudo"
fi
APT_UPDATED=0
ensure_apt_updated() {
  if [ "$APT_UPDATED" = "0" ]; then
    $SUDO apt-get update
    APT_UPDATED=1
  fi
}

cd "$APP_DIR"
if ! python3 -m venv --help >/dev/null 2>&1; then
  ensure_apt_updated
  $SUDO DEBIAN_FRONTEND=noninteractive apt-get install -y python3-venv python3-pip
fi
if ! command -v fswebcam >/dev/null 2>&1; then
  ensure_apt_updated
  $SUDO DEBIAN_FRONTEND=noninteractive apt-get install -y fswebcam || true
fi
python3 -m venv .venv
".venv/bin/pip" install --upgrade pip
".venv/bin/pip" install -r requirements.txt

$SUDO usermod -aG dialout "$USER_NAME" || true
$SUDO cp systemd/wardrobe-gateway.service /etc/systemd/system/"$SERVICE_NAME"
$SUDO systemctl daemon-reload
$SUDO systemctl enable "$SERVICE_NAME"
$SUDO systemctl restart "$SERVICE_NAME"

echo "Gateway installed."
echo "Touch screen URL: http://127.0.0.1:8000/"
echo "PC debug URL: http://<raspberry-pi-ip>:8000/debug"
