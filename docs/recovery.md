# Recovery

Чинится с live ISO (`docs/livecd.md`).

## Пересоздание ссылок

```sh
mount /dev/sda2 /mnt
mount /dev/sda1 /mnt/boot
venim repair --root /mnt
```

`repair` читает `/mnt/venim/db/packages.db`, пересоздаёт симлинки в `/mnt/usr` и юниты. Флаги: `--dry-run` (показать без записи), `--json`.

## Проверка дисков

```sh
fsck -y /dev/sda2
btrfs check /dev/sda2   # если btrfs
mount -o remount,rw /mnt
```

Только после чистой fsck делать repair.

## Chroot

```sh
mount --bind /dev /mnt/dev
mount --bind /proc /mnt/proc
mount --bind /sys /mnt/sys
chroot /mnt /bin/bash
venim verify | head -30
venim doctor
```

## Типовые случаи

- `kernel panic: can't mount root` — проверить LABEL в loader entry, пересобрать initramfs;
- пустой `/usr/bin` — `repair`, затем `doctor`;
- `verify` сыпет расхождениями — переустановить пакеты: `venim install --binary <name>`;
- БД битая — бэкап `/venim/db/packages.db.bak` (делает каждый `upgrade`), восстановить и повторить `repair`.
