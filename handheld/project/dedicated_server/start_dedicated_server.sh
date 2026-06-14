#!/bin/bash
# Script to start a Minecraft PE dedicated server.
# Supports foreground console mode and background mode.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/dedicated_server/build}"
BIN_PATH="$BUILD_DIR/mcpe_server"

if [ ! -x "$BIN_PATH" ]; then
  echo "mcpe_server binary not found at $BIN_PATH"
  echo "Build with:"
  echo "  cd $BUILD_DIR && mkdir -p build && cd build && cmake .. && make"
  exit 1
fi

PORT="${PORT:-19132}"
LEVELDIR="${LEVELDIR:-world}"
CACHEPATH="${CACHEPATH:-$SCRIPT_DIR/.mcpe_cache}"
EXTERNALPATH="${EXTERNALPATH:-$SCRIPT_DIR/.mcpe_external}"
GAMEMODE="${GAMEMODE:-survival}"
WORLDTYPE="${WORLDTYPE:-infinite}"
DIFFICULTY="${DIFFICULTY:-normal}"
RUNMODE="${RUNMODE:-console}"
CONFIG="${CONFIG:-$SCRIPT_DIR/server.properties}"

mkdir -p "$CACHEPATH" "$EXTERNALPATH"

if [ ! -f "$CONFIG" ]; then
  cat > "$CONFIG" <<EOF
# Run mode: console|background
runmode=$RUNMODE

port=$PORT
leveldir=$LEVELDIR
cachepath=$CACHEPATH
externalpath=$EXTERNALPATH
gamemode=$GAMEMODE
worldtype=$WORLDTYPE
difficulty=$DIFFICULTY
EOF
fi

# Allow config file to override env defaults.
if grep -qE '^runmode=' "$CONFIG"; then
  RUNMODE="$(grep -E '^runmode=' "$CONFIG" | tail -n1 | cut -d'=' -f2- | tr -d '[:space:]' | tr '[:upper:]' '[:lower:]')"
fi

WORLD_PATH="$EXTERNALPATH/$LEVELDIR"
echo "Using config: $CONFIG"
echo "World path: $WORLD_PATH"

if [ "$RUNMODE" = "background" ]; then
  nohup "$BIN_PATH" --config "$CONFIG" > "$SCRIPT_DIR/mcpe_server.log" 2>&1 &
  PID=$!
  echo "Started mcpe_server (PID=$PID) in background"
  echo "Log: $SCRIPT_DIR/mcpe_server.log"
else
  echo "Starting mcpe_server in console mode (Ctrl+C to stop)"
  exec "$BIN_PATH" --config "$CONFIG"
fi
