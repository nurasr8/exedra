#!/bin/bash
# Exedra system installer. Run from the live CD as root.
# Usage: install-exedra.sh --disk /dev/vda [--hostname H] [--user U] [--yes]
set -e
DISK=""; HOST=exedra; USER=exedra; YES=0
while [ $# -gt 0 ]; do
  case "$1" in
    --disk) DISK="$2"; shift 2;;
    --hostname) HOST="$2"; shift 2;;
    --user) USER="$2"; shift 2;;
    --yes) YES=1; shift;;
    *) echo "usage: $0 --disk /dev/vdX [--hostname H] [--user U] [--yes]"; exit 1;;
  esac
done
[ -n "$DISK" ] || { echo "need --disk"; exit 1; }
[ "$(id -u)" = 0 ] || { echo "run as root"; exit 1; }
if [ "$YES" != 1 ]; then
  echo "ALL DATA ON $DISK WILL BE ERASED. Continue? [y/N]"
  read -r a; [ "$a" = y ] || exit 1
fi
ESP="${DISK}1"; ROOTPART="${DISK}2"
case "$DISK" in *[0-9]) ESP="${DISK}p1"; ROOTPART="${DISK}p2";; esac

sfdisk --wipe always "$DISK" <<EOF
label: gpt
,512M,U
;
EOF
command -v partprobe >/dev/null && partprobe "$DISK" || partx -u "$DISK"
modprobe vfat ext4 2>/dev/null || true
mkfs.vfat -F32 -n EXEDRAESP "$ESP" >/dev/null
mkfs.ext4 -q -L EXEDRA "$ROOTPART"
MNT=/mnt/exedra-install
mkdir -p "$MNT"
mount "$ROOTPART" "$MNT"
mkdir -p "$MNT/boot"
mount "$ESP" "$MNT/boot"

tar -C / --one-file-system --exclude=./proc --exclude=./sys --exclude=./dev \
  --exclude=./run --exclude=./tmp --exclude=./mnt --exclude=./iso --exclude=./lower \
  --exclude=./ovl --exclude=./live --exclude=./boot --exclude='./var/tmp/*' -cf - . | tar -C "$MNT" -xf -
mkdir -p "$MNT"/{proc,sys,dev,run,tmp,mnt,boot}
chmod 755 "$MNT"/{proc,sys,dev,run,tmp,mnt}

ROOTUUID=$(blkid -s UUID -o value "$ROOTPART")
ESPUUID=$(blkid -s UUID -o value "$ESP")
cat > "$MNT/etc/fstab" <<EOF
UUID=$ROOTUUID / ext4 defaults 0 1
UUID=$ESPUUID /boot vfat defaults 0 2
EOF
echo "$HOST" > "$MNT/etc/hostname"
: > "$MNT/etc/machine-id"
grep -q "^$USER:" "$MNT/etc/passwd" || echo "$USER:x:1001:1001::/home/$USER:/bin/bash" >> "$MNT/etc/passwd"
grep -q "^$USER:" "$MNT/etc/group" || echo "$USER:x:1001:" >> "$MNT/etc/group"
grep -q "^$USER:" "$MNT/etc/shadow" || echo "$USER:!:20000:0:99999:7:::" >> "$MNT/etc/shadow"
mkdir -p "$MNT/home/$USER"

bootctl --esp-path="$MNT/boot" install >/dev/null
mkdir -p "$MNT/boot/loader/entries"
cat > "$MNT/boot/loader/loader.conf" <<'EOF'
timeout 5
default exedra
EOF
cat > "$MNT/boot/loader/entries/exedra.conf" <<EOF
title Exedra
linux /vmlinuz-exedra
initrd /initramfs-exedra.img
options root=UUID=$ROOTUUID console=ttyS0,115200 quiet
EOF
KV=$(ls "$MNT/usr/lib/modules/" | head -n1)
cp "$MNT/usr/lib/modules/$KV/vmlinuz" "$MNT/boot/vmlinuz-exedra"
cp "$MNT/usr/lib/modules/$KV/vmlinuz" "$MNT/boot/vmlinuz"
cp /boot/initramfs-exedra.img "$MNT/boot/initramfs-exedra.img"
if [ -f "$MNT/boot/initramfs-exedra.img" ]; then
  echo "boot files ok"
else
  echo "no initramfs in image, aborting" >&2
  exit 1
fi
chown 1001:1001 "$MNT/home/$USER"

umount "$MNT/boot"
umount "$MNT"
rmdir "$MNT"
echo "installed on $DISK. reboot without the ISO."
