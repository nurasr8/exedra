#!/bin/bash
# Assemble live rootfs: seed upstream userspace, preinstall hello-venim via venim.
# EDITION=core|desktop, default core.
# Everything runs under fakeroot so the image gets real root ownership
# and setuid bits (host build user is unprivileged).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EDITION="${EDITION:-core}"
case "$EDITION" in
  core) RFS="$ROOT/live/rootfs" ;;
  desktop) RFS="$ROOT/live/rootfs-desktop" ;;
  *) echo "unknown edition: $EDITION" >&2; exit 1 ;;
esac
export EDITION
STATE="$ROOT/live/fakeroot-$EDITION.save"
rm -f "$STATE"
export OUT="$RFS"
fakeroot -s "$STATE" bash "$ROOT/tools/seed-rootfs.sh"
fakeroot -i "$STATE" -s "$STATE" bash -c '
if [ -f "$ROOT/initramfs.img" ]; then
  cp "$ROOT/initramfs.img" "$RFS/boot/initramfs-exedra.img"
else
  echo "warning: no initramfs.img, installer will refuse" >&2
fi' ROOT="$ROOT" RFS="$RFS"
if [ "$EDITION" = desktop ]; then
  fakeroot -i "$STATE" -s "$STATE" bash "$ROOT/tools/overlay-desktop.sh" "$RFS"
fi
fakeroot -i "$STATE" -s "$STATE" env VENIM_ROOT="$RFS" python3 "$ROOT/bin/venim" --repo "$ROOT" install hello-venim
echo "live $EDITION rootfs at $RFS"
