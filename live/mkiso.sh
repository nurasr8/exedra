#!/bin/bash
# Build hybrid UEFI+BIOS ISO (systemd-boot + isolinux) from an edition rootfs.
# EDITION=core|desktop, default core.
# Needs: kernel at <rootfs>/boot/vmlinuz, initramfs.img, systemd-bootx64.efi on host.
set -e
VER="${1:-0.1.0}"
EDITION="${EDITION:-core}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
case "$EDITION" in
  core) RFS="$ROOT/live/rootfs"; TAG=Core ;;
  desktop) RFS="$ROOT/live/rootfs-desktop"; TAG=Desktop ;;
  *) echo "unknown edition: $EDITION" >&2; exit 1 ;;
esac
ISO="$ROOT/Exedra-$TAG-$VER-x86_64.iso"
WORK="$ROOT/live/iso-work-$EDITION"
for t in mksquashfs xorriso mkfs.vfat mcopy; do
  command -v $t >/dev/null || { echo "need $t" >&2; exit 1; }
done
[ -f "$RFS/boot/vmlinuz" ] || { echo "no kernel at $RFS/boot/vmlinuz" >&2; exit 1; }
[ -f "$ROOT/initramfs.img" ] || { echo "no initramfs.img (run initramfs/mkinitramfs.sh)" >&2; exit 1; }
BOOTX64="$(find /usr/lib/systemd/boot/efi /usr/share/systemd/boot -name systemd-bootx64.efi 2>/dev/null | head -n1)"
[ -n "$BOOTX64" ] || { echo "no systemd-bootx64.efi on host (install systemd)" >&2; exit 1; }
rm -rf "$WORK"
mkdir -p "$WORK/esp/EFI/BOOT" "$WORK/esp/loader/entries" "$WORK/live"
cp "$RFS/boot/vmlinuz" "$WORK/esp/vmlinuz-exedra"
cp "$ROOT/initramfs.img" "$WORK/esp/initramfs-exedra.img"
cp "$BOOTX64" "$WORK/esp/EFI/BOOT/BOOTX64.EFI"
cp "$ROOT/boot/loader/loader.conf" "$WORK/esp/loader/"
cp "$ROOT/boot/loader/entries/exedra.conf" "$WORK/esp/loader/entries/"
mksquashfs "$RFS" "$WORK/live/rootfs.squashfs" -comp xz
cp "$RFS/boot/vmlinuz" "$WORK/vmlinuz-exedra"
cp "$ROOT/initramfs.img" "$WORK/initramfs-exedra.img"
mkdir -p "$WORK/isolinux"
cp "$ROOT/boot/bios/isolinux.bin" "$ROOT/boot/bios/ldlinux.c32" "$WORK/isolinux/"
cp "$ROOT/boot/bios/isolinux.cfg" "$WORK/isolinux/isolinux.cfg"
dd if=/dev/zero of="$WORK/esp.img" bs=1M count=64 status=none
mkfs.vfat -n EXEDRAESP "$WORK/esp.img" >/dev/null
mcopy -s -i "$WORK/esp.img" "$WORK/esp/"* ::/
xorriso -as mkisofs -o "$ISO" -V EXEDRA -J -r \
  -isohybrid-mbr "$ROOT/boot/bios/isohdpfx.bin" \
  -b isolinux/isolinux.bin -c isolinux/boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table \
  -eltorito-alt-boot -e esp.img -no-emul-boot -isohybrid-gpt-basdat "$WORK"
echo "wrote $ISO"
