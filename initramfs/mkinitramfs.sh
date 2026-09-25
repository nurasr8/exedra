#!/bin/bash
# Minimal initramfs: loop-mount squashfs from ISO, switch_root to systemd.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RFS="$ROOT/live/rootfs"
OUT="${1:-$ROOT/initramfs.img}"
KV="$(ls "$RFS/usr/lib/modules/" | head -n1)"
[ -n "$KV" ] || { echo "no modules in $RFS" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK"/{bin,sbin,etc,proc,sys,dev,iso,newroot}
mkdir -p "$WORK/lib/modules/$KV"

copy_bin() {
  local src="$RFS$1" dst="$WORK$1"
  mkdir -p "$(dirname "$dst")"
  cp -aL "$src" "$dst"
  for lib in $(ldd "$src" 2>/dev/null | grep -o '/[^ ]*' || true); do
    mkdir -p "$WORK$(dirname "$lib")"
    cp -anL "$lib" "$WORK$lib" 2>/dev/null || true
  done
}

copy_bin /bin/bash
copy_bin /bin/mount
copy_bin /bin/umount
copy_bin /usr/bin/blkid
copy_bin /usr/bin/mkdir
copy_bin /usr/sbin/modprobe
copy_bin /usr/sbin/switch_root
ln -sf bash "$WORK/bin/sh"

for ko in $(cd "$RFS/lib/modules/$KV" && find kernel -name '*.ko*' | grep -E 'loop|isofs|squashfs|sr_mod|cdrom|ata_piix|ata_generic|libata|scsi_mod|sd_mod|sg_mod|virtio|virtio_blk|virtio_pci|virtio_ring|virtio_net|uhci|ehci|xhci|usb_storage|uas|libahci|ahci|overlay|e1000|e1000e|r8169|igb'); do
  mkdir -p "$WORK/lib/modules/$KV/$(dirname "$ko")"
  cp -a "$RFS/lib/modules/$KV/$ko" "$WORK/lib/modules/$KV/$ko"
done
depmod -b "$WORK" "$KV" 2>/dev/null || true

cat > "$WORK/init" <<'EOF'
#!/bin/sh
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devtmpfs dev /dev
for m in loop sr_mod cdrom ata_piix ata_generic squashfs isofs virtio_blk virtio_net e1000 e1000e overlay; do
  modprobe $m 2>/dev/null || true
done
ROOT=$(blkid -L EXEDRA 2>/dev/null)
ROOT=${ROOT%%$'\n'*}
[ -n "$ROOT" ] || { echo "Exedra: root ISO not found"; sh; }
mount -t iso9660 -o ro "$ROOT" /iso
mkdir -p /lower /ovl
mount -o loop,ro /iso/live/rootfs.squashfs /lower
mount -t tmpfs tmpfs /ovl
mkdir -p /ovl/upper /ovl/work
mount -t overlay overlay -o lowerdir=/lower,upperdir=/ovl/upper,workdir=/ovl/work /newroot
mkdir -p /newroot/run
mount --move /dev /newroot/dev
mount --move /proc /newroot/proc
mount --move /sys /newroot/sys
exec /usr/sbin/switch_root /newroot /usr/lib/systemd/systemd
EOF
chmod +x "$WORK/init"
(cd "$WORK" && find . -print0 | cpio --quiet -H newc -o --null | gzip -9 > "$OUT")
echo "wrote $OUT ($(du -h "$OUT" | cut -f1))"
