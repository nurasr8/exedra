#!/bin/bash
# Assemble live rootfs: seed upstream userspace, preinstall hello-venim via venim.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
bash "$ROOT/tools/seed-rootfs.sh"
VENIM_ROOT="$ROOT/live/rootfs" python3 "$ROOT/bin/venim" --repo "$ROOT" install hello-venim
echo "live rootfs at $ROOT/live/rootfs"
