#!/bin/bash
set -e
ISO="${1:-Exedra-*-x86_64.iso}"
KVM=""
[ -e /dev/kvm ] && KVM="-enable-kvm"
BIOS=""
for f in /usr/share/ovmf/x64/OVMF.fd /usr/share/edk2-ovmf/x64/OVMF.fd; do
  [ -f "$f" ] && BIOS="$f" && break
done
if [ -n "$BIOS" ]; then
  exec qemu-system-x86_64 -m 2048 -smp 2 $KVM \
    -bios "$BIOS" -cdrom $ISO -boot d -serial stdio -display none
fi
CODE="$(ls /usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd /usr/share/ovmf/x64/OVMF_CODE.4m.fd 2>/dev/null | head -n1)"
VARS_SRC="$(ls /usr/share/edk2-ovmf/x64/OVMF_VARS.4m.fd 2>/dev/null | head -n1)"
if [ -n "$CODE" ] && [ -n "$VARS_SRC" ]; then
  VARS="$(mktemp /tmp/opencode/ovmf-vars-XXXX.fd)"
  cp "$VARS_SRC" "$VARS"
  exec qemu-system-x86_64 -m 2048 -smp 2 $KVM \
    -drive if=pflash,format=raw,readonly=on,file="$CODE" \
    -drive if=pflash,format=raw,file="$VARS" \
    -cdrom $ISO -boot d -serial stdio -display none
fi
echo "no OVMF found, UEFI boot needs edk2-ovmf" >&2
exit 1
