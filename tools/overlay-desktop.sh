#!/bin/bash
# Desktop overlay on top of a seeded rootfs: SDDM autologin, graphical target,
# NetworkManager instead of networkd, PipeWire user units, installer shortcut.
set -e
OUT="${1:?usage: overlay-desktop.sh <rootfs>}"

rm -f "$OUT/etc/systemd/system/multi-user.target.wants/systemd-networkd.service"
rm -f "$OUT/etc/systemd/system/multi-user.target.wants/systemd-resolved.service"
mkdir -p "$OUT/etc/systemd/system/multi-user.target.wants"
ln -sf /usr/lib/systemd/system/NetworkManager.service \
  "$OUT/etc/systemd/system/multi-user.target.wants/NetworkManager.service"
ln -sf /usr/lib/systemd/system/sddm.service \
  "$OUT/etc/systemd/system/display-manager.service"
ln -sf /usr/lib/systemd/system/graphical.target \
  "$OUT/etc/systemd/system/default.target"

mkdir -p "$OUT/etc/sddm.conf.d"
cat > "$OUT/etc/sddm.conf.d/autologin.conf" <<'EOF'
[Autologin]
User=exedra
Session=plasmax11.desktop
EOF
mkdir -p "$OUT/etc/sudoers.d"
echo 'exedra ALL=(ALL) NOPASSWD: ALL' > "$OUT/etc/sudoers.d/live"
chmod 440 "$OUT/etc/sudoers.d/live"

mkdir -p "$OUT/etc/systemd/user/default.target.wants"
for u in pipewire.socket wireplumber.service pipewire-pulse.socket; do
  if [ -e "$OUT/usr/lib/systemd/user/$u" ]; then
    ln -sf "/usr/lib/systemd/user/$u" "$OUT/etc/systemd/user/default.target.wants/$u"
  fi
done

mkdir -p "$OUT/usr/share/applications"
cat > "$OUT/usr/share/applications/install-exedra.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Install Exedra
Exec=sudo -E calamares
Icon=drive-harddisk
Categories=System;
EOF

mkdir -p "$OUT/etc/calamares"
cp -r "$ROOT/desktop/calamares/"* "$OUT/etc/calamares/"

mkdir -p "$OUT/etc/xdg/autostart"
cat > "$OUT/etc/xdg/autostart/exedra-xhost.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Exedra X access
Exec=xhost +SI:localuser:root
NoDisplay=true
EOF
echo "desktop overlay done in $OUT"
