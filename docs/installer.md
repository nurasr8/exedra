# Installer

CLI: `install-exedra --disk /dev/sdX --hostname H --user U --yes`.
TUI: `install-exedra-tui` (needs `dialog`, guided: disk, hostname, user).
Desktop: shortcut «Install Exedra» (konsole + sudo).

Steps: GPT (ESP 512M + root), mkfs, copy live tree (tar, without /boot),
fstab by UUID, hostname, machine-id reset, user, systemd-boot via bootctl,
kernel + initramfs into ESP, loader entry with `root=UUID=`.

Verified in QEMU: install to blank disk → boot from disk → login, venim,
network, 0 failed units.

## Secure Boot note

Live ISO boots signed (Exedra db key, see `docs/secureboot.md`). The system
written to disk gets an *unsigned* bootloader copy: either disable Secure
Boot on first boot of the installed system, or sign it yourself after
install (chroot + `sbsign`) and enroll your key in firmware.
