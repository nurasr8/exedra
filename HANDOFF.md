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

## Быстрые команды

```sh
bash tools/smoke-test.sh
EDITION=desktop bash live/mklive.sh && EDITION=desktop bash live/mkiso.sh
bash tools/run-qemu.sh Exedra-Desktop-0.1.0-x86_64.iso
python3 tools/qemu-expect.py 590 <steps> <log> -- qemu-system... (см. историю)
git log --oneline | head
```
