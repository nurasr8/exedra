# Installer

CLI: `install-exedra --disk /dev/sdX --hostname H --user U --yes`.
TUI: `install-exedra-tui` (needs `dialog`, guided: disk, hostname, user).
Desktop: shortcut «Install Exedra» (konsole + sudo).

Steps: GPT (ESP 512M + root), mkfs, copy live tree (tar, without /boot),
fstab by UUID, hostname, machine-id reset, user, systemd-boot via bootctl,
kernel + initramfs into ESP, loader entry with `root=UUID=`.

Verified in QEMU: install to blank disk → boot from disk → login, venim,
network, 0 failed units.
