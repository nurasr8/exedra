#!/bin/bash
# Generate Exedra Secure Boot keys (PK/KEK/db). PRIVATE — never commit keys/.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
KEYS="$HERE/keys"
mkdir -p "$KEYS"
[ -f "$KEYS/db.key" ] && { echo "keys already exist in $KEYS"; exit 0; }
uuidgen 2>/dev/null > "$KEYS/GUID.txt" || cat /proc/sys/kernel/random/uuid > "$KEYS/GUID.txt"
for k in PK KEK db; do
  openssl req -newkey rsa:2048 -nodes -keyout "$KEYS/$k.key" -new -x509 \
    -sha256 -days 3650 -out "$KEYS/$k.crt" -subj "/CN=Exedra Secure Boot $k/"
  openssl x509 -in "$KEYS/$k.crt" -outform DER -out "$KEYS/$k.cer"
done
if command -v cert-to-efi-sig-list >/dev/null; then
  for k in PK KEK db; do
    cert-to-efi-sig-list -g "$(cat "$KEYS/GUID.txt")" "$KEYS/$k.crt" "$KEYS/$k.esl"
  done
  echo "wrote .esl lists for firmware enrollment"
fi
chmod 600 "$KEYS"/*.key
echo "keys in $KEYS (do NOT commit)"
