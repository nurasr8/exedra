#!/bin/bash
# Guided TUI installer for Exedra. Needs `dialog` (in Core).
set -e
[ "$(id -u)" = 0 ] || { echo "run as root"; exit 1; }
command -v dialog >/dev/null || { echo "need dialog"; exit 1; }

DISKS=()
while read -r name size model; do
  DISKS+=("$name" "$size $model")
done < <(lsblk -dnpo NAME,SIZE,MODEL 2>/dev/null | grep -vE 'rom|loop')

[ ${#DISKS[@]} -gt 0 ] || { dialog --msgbox "No disks found." 6 40; exit 1; }
DISK=$(dialog --stdout --title "Exedra install" --menu "Target disk (DATA WILL BE ERASED):" 15 60 6 "${DISKS[@]}") || exit 1
HOST=$(dialog --stdout --title "Hostname" --inputbox "Hostname:" 8 40 exedra) || exit 1
USER=$(dialog --stdout --title "User" --inputbox "Main user:" 8 40 exedra) || exit 1
dialog --yesno "Install Exedra on $DISK?\nHost: $HOST  User: $USER\nALL DATA ON $DISK WILL BE ERASED." 10 55 || exit 1

clear
install-exedra --disk "$DISK" --hostname "$HOST" --user "$USER" --yes
echo
echo "Done. Remove the ISO and reboot."
