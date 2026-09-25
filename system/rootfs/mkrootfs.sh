#!/bin/bash
# Build minimal rootfs in ./rootfs-out (override with ROOTFS_OUT).
set -e
OUT="${ROOTFS_OUT:-rootfs-out}"
mkdir -p "$OUT"/{boot,dev,etc,home,venim,proc,root,run,sys,tmp,usr/bin,var/lib,var/log}
mkdir -p "$OUT"/venim/{packages,build,cache,db,sources}
cp bin/venim "$OUT/usr/bin/venim" 2>/dev/null || cp "$(pwd)/bin/venim" "$OUT/usr/bin/venim"
cat > "$OUT/etc/passwd" <<'EOF'
root:x:0:0:root:/root:/bin/bash
exedra:x:1000:1000::/home/exedra:/bin/bash
EOF
cat > "$OUT/etc/group" <<'EOF'
root:x:0:
exedra:x:1000:
EOF
echo "exedra-core" > "$OUT/etc/hostname"
echo "rootfs ready in $OUT"
