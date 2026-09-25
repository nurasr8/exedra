#!/bin/bash
# Sign ESP binaries in place with the Exedra db key. Usage: sign.sh <esp-dir>
set -e
ESP="${1:?usage: sign.sh <esp-dir>}"
HERE="$(cd "$(dirname "$0")" && pwd)"
KEYS="$HERE/keys"
[ -f "$KEYS/db.key" ] || { echo "no keys, run gen-keys.sh (building unsigned)"; exit 0; }
SBSIGN=""
for c in sbsign "$HOME/sbtools/usr/bin/sbsign" /usr/bin/sbsign; do
  command -v "$c" >/dev/null 2>&1 || [ -x "$c" ] || continue
  SBSIGN="$c"; break
done
[ -n "$SBSIGN" ] || { echo "no sbsign, building unsigned"; exit 0; }
signed=0
for f in "$ESP/EFI/BOOT/BOOTX64.EFI" "$ESP/vmlinuz-exedra"; do
  [ -f "$f" ] || continue
  "$SBSIGN" --key "$KEYS/db.key" --cert "$KEYS/db.crt" --output "$f.signed" "$f" >/dev/null
  mv "$f.signed" "$f"
  signed=$((signed+1))
done
echo "signed $signed EFI binaries with Exedra db key"
