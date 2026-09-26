#!/bin/bash
# Closure pass: extract host packages providing .so files missing in rootfs.
# Usage: bash tools/seed-fix-libs.sh [rootfs]
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/live/rootfs}"
CACHES="/var/cache/pacman/pkg $HOME/pkgcache"

have() { ldconfig -r "$OUT" -p 2>/dev/null | grep -q " $1 "; }

needed=$(mktemp)
find "$OUT/usr/bin" "$OUT/usr/sbin" "$OUT/usr/lib" -type f \
  ! -name '*.ko*' ! -name '*.py*' ! -name '*.a' 2>/dev/null \
  | while read -r f; do
      readelf -d "$f" 2>/dev/null | sed -n 's/.*NEEDED.*\[\(.*\)\]/\1/p'
    done | sort -u > "$needed"

fixed=0
while read -r so; do
  have "$so" && continue
  host="/usr/lib/$so"
  [ -e "$host" ] || host=$(ls /usr/lib/$so 2>/dev/null | head -n1)
  [ -n "$host" ] || { echo "HOST-MISS: $so"; continue; }
  pkg=$(pacman -Qo "$host" 2>/dev/null | sed 's/.*is owned by //; s/ .*//')
  [ -n "$pkg" ] || { echo "PKG-MISS: $so"; continue; }
  f=""
  for c in $CACHES; do
    f=$(ls -t "$c/$pkg"-[0-9]*.pkg.tar.zst 2>/dev/null | head -n1)
    [ -n "$f" ] && break
  done
  if [ -z "$f" ]; then echo "CACHE-MISS: $pkg ($so)"; continue; fi
  echo "ADD: $pkg (for $so)"
  bsdtar -xpf "$f" -C "$OUT" --exclude=.PKGINFO --exclude=.MTREE --exclude=.INSTALL
  fixed=$((fixed+1))
done < "$needed"
rm -f "$needed"
ldconfig -r "$OUT" 2>/dev/null || true
echo "fixed $fixed packages"
