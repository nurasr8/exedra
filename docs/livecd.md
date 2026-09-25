# LiveCD

Две редакции, общий фундамент (`editions/*.list`):

- Core: минимальный CLI/TTY образ, без графики → `Exedra-Core-<version>-x86_64.iso`
- Desktop: Core + KDE Plasma, SDDM, приложения → `Exedra-Desktop-<version>-x86_64.iso`

## Состав Core

- kernel + initramfs из `boot/`, `initramfs/`
- base: glibc, bash, coreutils, systemd, systemd-networkd, openssh, venim
- storage/recovery: sfdisk, mkfs, fsck, lvm2, mdadm, cryptsetup, smartmontools
- network: iproute2, iw, iwd, wpa_supplicant, ethtool, curl
- squashfs с `/venim/packages` базы
- UEFI systemd-boot + BIOS isolinux, Secure Boot не подписан — отключать при проверке

## Состав Desktop

Core + `editions/desktop.list`: Mesa, SDDM (автологин exedra), Plasma,
Konsole, Dolphin, Firefox, LibreOffice, PipeWire, NetworkManager, CUPS.
Графический таргет по умолчанию, ярлык «Install Exedra».

## Имена

`Exedra-Core-<version>-x86_64.iso` и `Exedra-Desktop-<version>-x86_64.iso`.

## Сборка

```sh
./tools/build live      # core: squashfs + initramfs
./tools/build iso       # core ISO
EDITION=desktop ./tools/build live
EDITION=desktop ./tools/build iso
./tools/run-qemu.sh     # проверка: qemu -m 2G -enable-kvm -bios OVMF -cdrom Exedra-*.iso
```

Логи в `out/build.log`. Повторная сборка переиспользует `/venim/cache`.

## Внутри live

Логин `root`, без пароля. Сеть поднимается NetworkManager. Установка на диск: `venim install base`, дальше `venim repair --root /mnt` не нужен — live сам раскладывает симлинки.

## Проверка образа

```sh
qemu-img info out/*.iso
./tools/run-qemu.sh out/Exedra-*.iso
```

В QEMU проверить: `venim list`, `ip a`, `journalctl -b | tail`.
