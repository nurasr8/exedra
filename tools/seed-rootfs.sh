#!/bin/bash
# Seed live/rootfs from host pacman cache (upstream bootstrap, no root needed).
# Usage: bash tools/seed-rootfs.sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/live/rootfs"
CACHE=/var/cache/pacman/pkg

SEED="filesystem glibc bash readline ncurses coreutils systemd util-linux shadow kmod
e2fsprogs iproute2 iputils procps-ng grep sed gawk tar gzip xz findutils less file
which nano curl ca-certificates openssl openssh pciutils usbutils linux python dbus systemd-sysvcompat dosfstools"

resolve() {
  local pkg="$1"
  pacman -Si "$pkg" 2>/dev/null | awk -F: '/^Depends On/{print $2}' | tr ' ' '\n' \
    | sed 's/[<>=].*//; s/^ *//; /^$/d' | sort -u
}

declare -A seen
queue=($SEED)
while [ ${#queue[@]} -gt 0 ]; do
  pkg="${queue[0]}"; queue=("${queue[@]:1}")
  [ -n "${seen[$pkg]:-}" ] && continue
  seen[$pkg]=1
  for d in $(resolve "$pkg"); do
    [ -n "${seen[$d]:-}" ] || queue+=("$d")
  done
done

mkdir -p "$OUT"
for pkg in "${!seen[@]}"; do
  f=$(ls -t "$CACHE/$pkg"-[0-9]*.pkg.tar.zst 2>/dev/null | head -n1)
  if [ -z "$f" ]; then echo "MISS: $pkg"; continue; fi
  bsdtar -xf "$f" -C "$OUT" --exclude=.PKGINFO --exclude=.MTREE --exclude=.INSTALL
done

# Exedra overlay
mkdir -p "$OUT"/{etc,root,home/exedra,venim/{packages,build,cache,db,sources},var/lib,var/log}
cp "$ROOT/bin/venim" "$OUT/usr/bin/venim"
cp "$ROOT/tools/install-exedra.sh" "$OUT/usr/sbin/install-exedra"
chmod +x "$OUT/usr/sbin/install-exedra"
mkdir -p "$OUT/usr/lib/venim"
cp -r "$ROOT/src/venim" "$OUT/usr/lib/venim/venim"
# Close missing .so providers, refresh linker cache
bash "$ROOT/tools/seed-fix-libs.sh" "$OUT"
# Hardware database (silences imds-generator warning on boot)
systemd-hwdb update --root="$OUT" --usr 2>/dev/null || systemd-hwdb update --root="$OUT" 2>/dev/null || true
# System users/groups from package fragments (build-time, /etc is ro on live)
systemd-sysusers --root="$OUT"
ldconfig -r "$OUT" 2>/dev/null || true
# root without password + exedra user, merged with sysusers output
sed -i 's/^root:x:/root::/' "$OUT/etc/passwd"
sed -i 's/^root:[^:]*:/root::/' "$OUT/etc/shadow"
grep -q '^exedra:' "$OUT/etc/passwd" || echo 'exedra:x:1000:1000::/home/exedra:/bin/bash' >> "$OUT/etc/passwd"
grep -q '^exedra:' "$OUT/etc/group" || echo 'exedra:x:1000:' >> "$OUT/etc/group"
if grep -q '^exedra:' "$OUT/etc/shadow"; then
  sed -i 's/^exedra:[^:]*:/exedra:!:/' "$OUT/etc/shadow"
else
  echo 'exedra:!:20000:0:99999:7:::' >> "$OUT/etc/shadow"
fi
if [ -f "$OUT/etc/gshadow" ]; then
  grep -q '^exedra:' "$OUT/etc/gshadow" || echo 'exedra:!::' >> "$OUT/etc/gshadow"
fi
chmod 640 "$OUT/etc/shadow" 2>/dev/null || true
chmod 640 "$OUT/etc/gshadow" 2>/dev/null || true
echo "exedra" > "$OUT/etc/hostname"
cat > "$OUT/etc/os-release" <<'EOF'
NAME="Exedra"
PRETTY_NAME="Exedra 0.1.0"
ID=exedra
VERSION="0.1.0"
VERSION_ID="0.1.0"
ANSI_COLOR="1;34"
HOME_URL="https://example.org/exedra"
EOF
cp "$OUT/etc/os-release" "$OUT/usr/lib/os-release"
KV="$(ls "$OUT/usr/lib/modules/" | head -n1)"
if [ -f "$OUT/usr/lib/modules/$KV/vmlinuz" ]; then
  mkdir -p "$OUT/boot"
  cp "$OUT/usr/lib/modules/$KV/vmlinuz" "$OUT/boot/vmlinuz"
fi
: > "$OUT/etc/machine-id"
ln -sf /usr/lib/systemd/systemd "$OUT/sbin/init" 2>/dev/null || ln -sf ../usr/lib/systemd/systemd "$OUT/sbin/init"
mkdir -p "$OUT/etc/systemd/system/getty.target.wants" "$OUT/etc/systemd/system/multi-user.target.wants"
ln -sf /usr/lib/systemd/system/getty@.service "$OUT/etc/systemd/system/getty.target.wants/getty@tty1.service"
ln -sf /usr/lib/systemd/system/serial-getty@.service "$OUT/etc/systemd/system/getty.target.wants/serial-getty@ttyS0.service"
ln -sf /usr/lib/systemd/system/systemd-networkd.service "$OUT/etc/systemd/system/multi-user.target.wants/systemd-networkd.service"
ln -sf /usr/lib/systemd/system/systemd-resolved.service "$OUT/etc/systemd/system/multi-user.target.wants/systemd-resolved.service"
mkdir -p "$OUT/etc/systemd/network"
cat > "$OUT/etc/systemd/network/20-wired.network" <<'EOF'
[Match]
Name=en* eth*
[Network]
DHCP=yes
EOF
echo "seeded $(echo ${!seen[@]} | wc -w) packages in $OUT, $(du -sh "$OUT" | cut -f1)"
