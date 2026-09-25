# Boot

Цепочка: `Bootloader → kernel → initramfs → systemd → TTY`.

## Этапы

1. UEFI systemd-boot (`boot/loader/`): выбирает kernel + initramfs с ESP.
2. kernel монтирует initramfs как root, запускает `/init` (скрипт в `initramfs/`).
3. `/init`: монтирует реальный root по LABEL, делает `switch_root`.
4. systemd (PID 1): `default.target` → `multi-user.target`, поднимает TTY на tty1.

## loader entries

```ini
# boot/loader/entries/exedra.conf
title Exedra
linux /vmlinuz-exedra
initrd /initramfs-exedra.img
options root=LABEL=EXEDRA quiet
```

LABEL смотреть `blkid`. Файлы правятся руками, пересборки загрузчика не нужно.

## initramfs без dracut

Собирается `./tools/build live`: busybox + скрипт `/init` + модули ext4/btrfs/nvme/usb. Лишнего нет. Проверить содержимое:

```sh
lsinitrd out/initramfs.img | head -40
```

Частая ошибка — забытый модуль диска. Добавить в `initramfs/modules.list`, пересобрать.
