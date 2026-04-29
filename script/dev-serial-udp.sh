#!/usr/bin/env bash
set -euo pipefail

TTY="/tmp/ttyV0"
LOCAL_PORT="${1:-9001}"
REMOTE_HOST="${2:-127.0.0.1}"
REMOTE_PORT="${3:-9002}"

rm -f "$TTY"

echo "Creating virtual serial UDP bridge:"
echo "  Serial:     $TTY"
echo "  UDP listen: 0.0.0.0:$LOCAL_PORT"
echo "  UDP target: $REMOTE_HOST:$REMOTE_PORT"
echo
echo "Your C++ Server opens:"
echo "  $TTY"
echo
echo "Send UDP to serial:"
echo "  echo 'hello from udp' | nc -u 127.0.0.1 $LOCAL_PORT"
echo
echo "Receive UDP from serial:"
echo "  nc -ul $REMOTE_PORT"
echo

socat -d -d \
  pty,raw,echo=0,link="$TTY" \
  udp4-datagram:"$REMOTE_HOST":"$REMOTE_PORT",localport="$LOCAL_PORT",reuseaddr