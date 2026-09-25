#!/bin/bash
# Assemble live rootfs: seed upstream userspace, preinstall hello-venim via venim.
# EDITION=core|desktop, default core.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EDITION="${EDITION:-core}"
case "$EDITION" in
  core) RFS="$ROOT/live/rootfs" ;;
  desktop) RFS="$ROOT/live/rootfs-desktop" ;;
  *) echo "unknown edition: $EDITION" >&2; exit 1 ;;
esac
export EDITION
OUT="$RFS" bash "$ROOT/tools/seed-rootfs.sh"
if [ -f "$ROOT/initramfs.img" ]; then
  cp "$ROOT/initramfs.img" "$RFS/boot/initramfs-exedra.img"
else
  echo "warning: no initramfs.img, installer will refuse" >&2
fi
if [ "$EDITION" = desktop ]; then
  bash "$ROOT/tools/overlay-desktop.sh" "$RFS"
fi
VENIM_ROOT="$RFS" python3 "$ROOT/bin/venim" --repo "$ROOT" install hello-venim
echo "live $EDITION rootfs at $RFS"
