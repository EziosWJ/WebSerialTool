#!/usr/bin/env bash
set -euo pipefail

TTY="/tmp/ttyV0"
PORT="${1:-9000}"

rm -f "$TTY"

echo "Creating virtual serial port:"
echo "  Serial: $TTY"
echo "  TCP:    127.0.0.1:$PORT"
echo
echo "Your C++ Server opens:"
echo "  $TTY"
echo
echo "TCP client connects:"
echo "  nc 127.0.0.1 $PORT"
echo

socat -d -d \
  pty,raw,echo=0,link="$TTY" \
  tcp-listen:"$PORT",reuseaddr,fork