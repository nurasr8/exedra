# Calamares (Desktop)

Графический установщик Desktop-редакции. Конфиг — адаптированный ALCI
(`desktop/calamares/` → `/etc/calamares` через `tools/overlay-desktop.sh`).

## Поток

welcome → locale → keyboard → partition → users → summary → install → finished.

Install: mount, unpackfs (`/live/rootfs.squashfs` — init монтирует файл
в `/newroot/live/` через bind), machineid, fstab, locale, keyboard, users,
displaymanager (sddm), networkcfg, hwclock, shellprocess@bootloader
(bootctl + записи systemd-boot + kernel из модулей), shellprocess@exedra-cleanup
(убрать autologin и live-sudoers, включить wheel, удалить live-юзера exedra
если имя не совпало), umount.

## Сборка бинарника

Calamares только в AUR. Сборка без root через sysroot:

1. Deps из кэша: `kpmcore`, `libpwquality` + остальное из репозитория.
2. `cmake` с `CMAKE_PREFIX_PATH=~/aurbuild/sysroot/usr`.
3. `tools/pack-calamares.sh` → `~/pkgcache/calamares-3.4.2-1.pkg.tar.zst`,
   подхватывается сидом Desktop (`editions/desktop.list`).

Запуск в live: SDDM-автологин exedra (X11) + `xhost +SI:localuser:root`
через autostart, ярлык «Install Exedra» делает `sudo -E calamares`
(sudoers NOPASSWD только в live, cleanup его удаляет).
