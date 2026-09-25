# LiveCD

Минимальный CLI/TTY образ, без графики.

## Состав

- kernel + initramfs из `boot/`, `initramfs/`
- base: glibc, bash, coreutils, systemd, NetworkManager, openssh, venim
- squashfs с `/venim/packages` базы
- UEFI systemd-boot, Secure Boot не подписан — отключать при проверке

## Имя

`Exedra-<version>-x86_64.iso`, версия из `packages/base/version`.

## Сборка

```sh
./tools/build live      # squashfs + initramfs
./tools/build iso       # заворачивает в ISO
./tools/run-qemu.sh     # проверка: qemu -m 2G -enable-kvm -bios OVMF -cdrom out/*.iso
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
