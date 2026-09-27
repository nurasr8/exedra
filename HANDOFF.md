# HANDOFF — Exedra / venim (для следующей модели)

Дата: 2026-09-26. Репозиторий: `/home/veniam/Projects/Nexa Core` (git, ветка master).
PROMPT.MD / SKILL.MD / COREPR.MD / DESKTOPPR.MD — исходные ТЗ, не менять.

## Что это

Exedra — независимый Linux-дистрибутив (ядро+systemd из upstream Arch),
пакетник `venim`, DSL рецептов VenimBuild (`*.vnb`), пакеты в
`/venim/packages/<name>/<version>/`, интеграция симлинками в `/usr`.
Две редакции: Core (CLI) и Desktop (KDE Plasma + Calamares).

## Проверено и работает (не переделывать)

- `Exedra-Core-0.1.0-x86_64.iso` грузится (UEFI systemd-boot и BIOS isolinux),
  systemd → TTY, логин root без пароля, 0 failed-юнитов, сеть по DHCP, пинг.
- `venim --version`, `hello` → `Hello from Exedra!`, install/verify/remove.
- Установщик `install-exedra` + TUI: полный цикл ISO → установка на чистый
  диск → загрузка с диска → логин/venim/сеть/0 failed. Проверен в QEMU.
- Secure Boot: ключи `tools/secureboot/keys/` (GITIGNORED!), подпись ESP при
  сборке, `sbverify OK`, enrollment через `efi-updatevar` + `.auth`,
  загрузка с enforcement (`Secure boot enabled`, наш серт в integrity).
  VARS с ключами: только в QEMU-файле, в репо его нет.
- Firefox 145.0 ставится через venim и запускается (`Mozilla Firefox 145.0`).
- Calamares 3.4.2 собран из AUR без root (sysroot `~/aurbuild/sysroot`),
  конфиг адаптирован из ALCI, под Xvfb окно создаётся, 11 модулей грузятся.
- `bash tools/smoke-test.sh` — 8/8. `python3 tests/test_venim.py` — ок.

## Прерванная работа (продолжить с этого)

- Фоновая пересборка Desktop ISO (добавлен os-prober) убита рестартом.
  Продолжение:
  `EDITION=desktop bash live/mklive.sh && EDITION=desktop bash live/mkiso.sh`
  (~15 мин), затем QEMU-проверка Desktop (sddm active, автологин exedra,
  файлы calamares в `/etc/calamares`).
- Desktop ISO от 22:02 (2 ГБ) — старый, без os-prober и свежих правок.

## Грабли (уже наступали, не повторять)

- `pkill -f '...Nexa...'` убивает саму вызывающую команду (паттерн матчится
  на bash -c). Использовать `'[q]emu-system...'` или точные PID.
- Длинные сборки (>10 мин) — только через `setsid nohup ... & disown`,
  иначе смерть вместе с вызовом. QEMU-тесты — через `tools/qemu-expect.py`
  (формат шагов в шапке файла), не слипами.
- xorriso: для dual BIOS+UEFI нужен `-eltorito-alt-boot` перед `-e`,
  иначе EFI-запись молча теряется. Проверять `xorriso -report_el_torito`.
- initramfs: у ядра нет PATH — в `/init` только абсолютные пути/bin+export;
  нет `head/grep/cat` (только встроенные sh + скопированные бины);
  библиотеки копировать с `-L` (висячие симлинки = kernel panic).
- `systemd.volatile=yes` НЕ делает /etc перезаписываемым → свой overlay
  в init (lower=squashfs, upper/work на tmpfs). Метка EXEDRA и на ISO, и на
  установленном root — в init различать по fstype (iso9660 vs ext4).
- Рецепты binary-`install{}` пишут СРАЗУ финальный layout (`bin/`, `opt/`),
  ссылки относительные (`../opt/...`), иначе нормализация `usr/` ломает уровни.
- `link_package` — только относительные симлинки (ISO/установленная система).
- Пересборка сида аддитивна: каталоги с правами 555 (bluez) ломают
  повторный сид — в seed-rootfs.sh уже есть `chmod -R u+w`.
- Shadow root'а периодически возвращается в `*` — в конце seed-rootfs.sh
  стоит принудительная разблокировка, не убирать.
- `bsdtar --zstd` в этом окружении молча игнорируется — паковать через
  `bsdtar -cf + zstd` отдельно (см. tools/pack-calamares.sh).
- cmake кэширует LDFLAGS/CXXFLAGS — при смене флагов только чистая
  пересборка (`rm -rf build`).

## Важные пути вне репо

- `~/pkgcache/` — скачанные .pkg.tar.zst (сид ищет тут + /var/cache/pacman/pkg).
- `~/aurbuild/` — calamares: исходники, sysroot, build/, pkg/, cmake*.log.
- `~/sbtools/` — sbsign/sbverify, `~/sbtools2/` — efitools (cert-to-efi-sig-list).
- `/tmp/opencode/` — qemu-логи, шаги expect (*.steps), OVMF-vars, keys.img.
  Вытирается при рестарте сервера — важное коммитить/копировать.
- `VENIM_ROOT` (не NEX_ROOT), пути `/venim/...`, `live/rootfs*` в gitignore.

## Следующие задачи (по приоритету)

1. Добить Desktop ISO + QEMU-проверка (sddm, calamares-файлы, автологин).
2. Calamares end-to-end: установка через GUI в QEMU на чистый диск
   (интерактив не автоматизировать — кликать нечем; минимум: старт GUI
   в живой сессии под Xvfb/дисплеем, проверить страницы до partition).
3. GUI-установщик Qt с нуля НЕ нужен — Calamares и есть решение.
4. Долгое: сборка базы через venim (glibc/gcc/systemd из .vnb) вместо
   сида из Arch-кэша; NVIDIA-проприетарщина; shim для SB без enrollment.
5. Мелочь: ворнинг `switch_root /run`, `imds-generator hwdb` в логе загрузки.

## Дополнение 2026-09-26 (вечер)

- Panic `Attempted to kill init! exitcode=0x100` на реальном железе:
  в `/init` модпробbились только ata_piix/virtio, USB-флешка не видна
  мгновенно → blkid пуст → switch_root exit 1. Фикс (коммит 1e5197d):
  modprobe usb_storage/uas/ahci/nvme/sdhci, ожидание носителя до 30с,
  `usb-storage` в copy-паттерне через `usb[_-]storage` (дефис в имени!),
  вместо паники — `exec sh`. Учтено: xhci/sd_mod/ahci/usbcore — builtin,
  копировать не надо; в initramfs нет `seq` — только POSIX-циклы/встроенные.
- Консоль ядра теперь `tty0+ttyS0` (live entries, isolinux, install-exedra):
  на машинке без serial видно загрузку, serial-тесты не сломаны.
- Проверенный expect-сценарий Desktop: `tools/qemu-desktop.steps`
  (stty -echo, полный промпт, 1 команда на шаг — иначе эхо матчит EXPECT).
- QEMU Desktop: запуск через `-vga virtio` (kwin_wayland нужен DRM).
- Грабли снова: `pkill -f` с паттерном убил свой же шелл; /tmp/opencode
  вытерт рестартом сервера (~14:35) — важное держать в репо.
- Актуальный Desktop ISO + SHA256SUMS + README: `release/` (в gitignore).

## Дополнение 2026-09-26 (вечер 2)

- Calamares "не стартует" по клику: ВЕСЬ rootfs был распакован от uid 1000
  без setuid (`sudo: must be owned by uid 0 and have the setuid bit set`).
  Фикс: весь сид+оверлей+venim под fakeroot (`-s/-i` state в
  `~/.cache/exedra-fakeroot/`, т.к. fakeroot ломается на пробелах в пути!),
  распаковка `bsdtar -xpf`, mksquashfs под fakeroot. Проверка в QEMU:
  `sudo -n true` от exedra = 0. Заодно чинены установленные системы
  (наследуют владение из squashfs). Ярлык апстрима (pkexec) покрыт
  polkit-правилом `49-exedra-live.rules`.
- Проверенный сценарий: `tools/qemu-verify.steps`.
- Discover-стек в образе (бинарь `plasma-discover` + flatpak/fwupd),
  меню почищено от Qt-dev/Avahi/geo (NoDisplay в overlay).
- WiFi: в образе все firmware-сплиты, iw/iwd/wpa_supplicant/rfkill/NM,
  hostapd/v4l-utils/acpid. Broadcom-wl (DKMS) preinstall невозможен
  без root-сборки — только через USB-tethering + headers после установки.
- 2026-09-26 (ночь): venim переписан на C++17 (src-cpp/, cmake, sqlite3+
  curl+openssl), CLI/DB паритет с Python (проверено diff),
  Python-исходники удалены. Сборка: tools/build-venim.sh -> build-cpp/venim.
  Полный пайплайн и live-QEMU с C++-бинарником зелёные.
- 2026-09-27: venim умеет --no-check, install по URL/файлу, --repo URL
  через index.json (+команда index, ключ file, одиночные payloads).
  examples/ удалены как дубли packages/. hub/: site (plain-site сайт),
  recipes (209 .vnb без sha256 + index.json), repo (EN GitHub-репо:
  README/CONTRIBUTING/RECIPE-SPEC/template/LICENSE/CI). Проверены живые
  установки eza/jq/starship из новых рецептов. ISO пересобран, релиз обновлён.
  curl+openssl), CLI/DB паритет с Python (проверено diff),
  Python-исходники удалены. Сборка: tools/build-venim.sh -> build-cpp/venim.
  Полный пайплайн и live-QEMU с C++-бинарником зелёные.
- 2026-09-26 (вечер 3): WiFi не работал НЕ из-за драйверов, а из-за
  ОТСУТСТВИЯ modules.dep/modules.alias (Arch генерит хуком depmod,
  сид его не запускал) — modalias-автозагрузка была мертва целиком,
  не грузился НИ ОДИН драйвер. Фикс: depmod -b в seed-rootfs.sh.
  Проверено в QEMU: modprobe dummy insmod OK. Чужие дистры ничего
  особенного не везут — тот же mainline + depmod. RTL8852CE (rtw89),
  8821ce, ath12k, mt7925e, rtw8922ae — всё в mainline, всё в образе.

## Быстрые команды

```sh
bash tools/smoke-test.sh
EDITION=desktop bash live/mklive.sh && EDITION=desktop bash live/mkiso.sh
bash tools/run-qemu.sh Exedra-Desktop-0.1.0-x86_64.iso
python3 tools/qemu-expect.py 590 <steps> <log> -- qemu-system... (см. историю)
git log --oneline | head
```
