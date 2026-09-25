# PROJECT: Nexa Core

Создай с нуля независимый Linux-дистрибутив под названием **Nexa Core**.

Nexa Core НЕ является форком Arch Linux, Debian, Fedora, Ubuntu или другого Linux-дистрибутива.

Используй Linux kernel и systemd как базовые компоненты, но создай собственную систему пакетов, собственный формат инструкций, собственную структуру пакетов, инструменты сборки и package manager.

Главный приоритет проекта — простота, понятность, стабильность и реальная работоспособность.

НЕ усложняй архитектуру без необходимости.

---

# 1. ОСНОВНАЯ ЦЕЛЬ

Создать Linux-дистрибутив Nexa Core со следующими основными особенностями:

- Linux kernel;
- systemd;
- собственный package manager `nex`;
- собственный язык package recipes `NexBuild`;
- изолированное хранение пакетов в `/nex`;
- поддержка сборки из исходников;
- поддержка готовых официальных бинарников;
- автоматическая адаптация бинарников под структуру Nexa Core;
- минимальный Live CD;
- полноценный TTY recovery environment;
- документация;
- примеры большого количества реальных пакетов;
- отдельный установщик системы после завершения Live CD.

---

# 2. НЕ ПЫТАЙСЯ ПЕРЕПИСАТЬ ВЕСЬ LINUX

"Nexa Core с нуля" НЕ означает написание собственного:

- Linux kernel;
- GCC;
- glibc;
- systemd;
- binutils;
- coreutils.

Используй существующие upstream-проекты там, где это разумно.

Независимость Nexa Core должна быть в:

- собственной архитектуре дистрибутива;
- package manager;
- NexBuild;
- package layout;
- build system;
- repository/recipe system;
- инструментах;
- Live CD;
- системной интеграции.

---

# 3. БАЗОВЫЕ КОМПОНЕНТЫ

Используй:

- Linux kernel;
- systemd;
- glibc;
- GCC;
- binutils;
- Bash;
- coreutils;
- util-linux;
- kmod;
- iproute2;
- e2fsprogs;
- стандартные Linux userspace tools.

Минимальная система должна уметь:

- загружаться;
- запускать systemd как PID 1;
- запускать TTY;
- выполнять login;
- работать с сетью;
- монтировать файловые системы;
- выключаться;
- перезагружаться.

---

# 4. ПАКЕТНАЯ АРХИТЕКТУРА

Главная особенность Nexa Core:

**каждый пакет физически отделён от остальных пакетов.**

Все установленные пакеты находятся внутри:

/nex/packages/

Структура:

/nex/
├── packages/
├── build/
├── cache/
├── db/
└── sources/

Например:

/nex/packages/bash/5.3/
/nex/packages/nano/8.6/
/nex/packages/coreutils/9.x/

Каждый пакет имеет собственный root:

/nex/packages/<name>/<version>/

Например:

/nex/packages/nano/8.6/
├── bin/
│   └── nano
├── share/
└── ...

Другой пакет:

/nex/packages/bash/5.3/
├── bin/
│   └── bash
├── share/
└── ...

Файлы пакетов НЕ должны смешиваться.

---

# 5. НЕ УСЛОЖНЯТЬ ИЗОЛЯЦИЮ

Не создавай:

- generations;
- snapshots;
- сложную систему profiles;
- rollback architecture;
- сложные immutable generations.

Это НЕ является целью проекта.

Нужна простая модель:

PACKAGE
→ собственная директория
→ собственные файлы
→ зависимости на другие директории.

---

# 6. ВЕРСИИ

Используй:

/nex/packages/<name>/<version>/

Например:

/nex/packages/nano/8.5/
/nex/packages/nano/8.6/

Несколько версий одного пакета могут существовать одновременно.

Package manager должен понимать, какая версия используется системой.

---

# 7. СИСТЕМНАЯ ИНТЕГРАЦИЯ

Пользователь не должен запускать:

/nex/packages/nano/8.6/bin/nano

Вместо этого создавай простую системную интеграцию.

Например:

/usr/bin/nano
→ /nex/packages/nano/8.6/bin/nano

или другой простой механизм.

Главные требования:

- физический файл пакета остаётся в `/nex`;
- `/usr/bin` не содержит копий package files;
- пакеты не смешиваются;
- удаление пакета не удаляет чужие файлы.

Не создавай сложную profile system.

---

# 8. СИСТЕМНЫЕ КАТАЛОГИ

Не нужно помещать абсолютно весь Linux filesystem внутрь `/nex`.

Сохрани стандартные каталоги:

/boot
/dev
/etc
/home
/lib
/media
/mnt
/opt
/proc
/root
/run
/sbin
/sys
/tmp
/usr
/var

Но package files должны храниться в:

/nex/packages/

Runtime directories могут использовать стандартный Linux layout.

Например:

/etc
/var/lib
/var/log
/run

могут содержать системные runtime/configuration данные.

---

# 9. ЗАВИСИМОСТИ

Пакеты могут зависеть друг от друга.

Например:

nano
→ ncurses
→ glibc

При этом:

/nex/packages/nano/8.6/

/nex/packages/ncurses/<version>/

/nex/packages/glibc/<version>/

остаются отдельными.

Package manager должен поддерживать:

- dependencies;
- build dependencies;
- optional dependencies;
- conflicts;
- provides.

---

# 10. PACKAGE MANAGER

Название:

**nex**

Основные команды:

nex install <package>
nex remove <package>
nex update
nex upgrade
nex search <query>
nex info <package>
nex list
nex depends <package>
nex provides <file>
nex verify <package>
nex clean
nex build
nex doctor
nex repair

Также:

nex --help
nex --version

Добавь:

--source
--binary
--verbose
--quiet
--json

CLI должен быть простым и Unix-like.

---

# 11. ДВА СПОСОБА ПОЛУЧЕНИЯ ПАКЕТОВ

Nexa Core должна поддерживать:

## Source packages

Получить исходный код:

recipe
→ source
→ build
→ install

## Binary packages

Получить готовый официальный бинарник:

official binary
→ verify
→ extract
→ adapt
→ validate
→ install

Оба способа в итоге должны создавать одинаковую структуру:

/nex/packages/<name>/<version>/

---

# 12. SOURCE BUILD

Пример:

nex install nano

Package manager:

1. получает NexBuild recipe;
2. определяет зависимости;
3. скачивает source;
4. проверяет SHA-256;
5. создаёт build environment;
6. собирает программу;
7. устанавливает её в staging directory;
8. проверяет результат;
9. переносит в:

/nex/packages/nano/8.6/

---

# 13. СОБСТВЕННЫЙ ЯЗЫК NEXBUILD

Создай собственный DSL:

**NexBuild**

Файлы:

*.nxb

Пример:

package "nano" {
    version = "8.6"

    description = "A small text editor"

    license = "GPL-3.0"

    homepage = "https://www.nano-editor.org/"

    source {
        url = "https://..."
        sha256 = "..."
    }

    depends {
        "glibc"
        "ncurses"
    }

    makedepends {
        "gcc"
        "make"
    }

    build {
        command "./configure --prefix=/usr"
        command "make"
    }

    install {
        command "make DESTDIR=${DESTDIR} install"
    }
}

Синтаксис может отличаться, если ты считаешь другой вариант лучше.

Не копируй PKGBUILD один в один.

---

# 14. NEXBUILD ДОЛЖЕН ПОДДЕРЖИВАТЬ

Минимально:

package
version
description
license
homepage
source
depends
makedepends
provides
conflicts
prepare
build
check
install
environment
binary
patch
service

Переменные:

${name}
${version}
${srcdir}
${builddir}
${destdir}
${prefix}

Не превращай DSL в огромный полноценный язык программирования без необходимости.

---

# 15. BINARY PACKAGES

Nexa Core может устанавливать готовые бинарники.

Например:

package "example" {
    version = "1.0"

    binary {
        url = "https://official-project.org/example.tar.xz"
        sha256 = "..."
        format = "tar.xz"
    }
}

Но бинарник НЕЛЬЗЯ просто распаковать в `/usr`.

Он должен пройти:

DOWNLOAD
→ VERIFY
→ EXTRACT
→ ANALYZE
→ PATCH
→ NEXA LAYOUT
→ VALIDATE
→ INSTALL

---

# 16. NEX BINARY ADAPTER

Создай отдельный компонент:

**Nex Binary Adapter**

Он должен адаптировать официальные Linux binary packages к Nexa Core.

Например, официальный архив:

bin/
lib/
share/

становится:

/nex/packages/example/1.0/
├── bin/
├── lib/
└── share/

---

# 17. АДАПТАЦИЯ ПУТЕЙ

Если сторонняя программа ожидает:

/usr/bin
/usr/lib
/usr/share

Adapter должен уметь корректно адаптировать её к Nexa Core.

Но НЕ используй тупую глобальную замену строк во всех бинарниках.

Разбирай формат файла.

Поддержи:

- ELF;
- shell scripts;
- Python scripts;
- desktop files;
- configuration files;
- symlinks.

---

# 18. ELF

Для ELF анализируй:

- interpreter;
- RPATH;
- RUNPATH;
- DT_NEEDED;
- shared library dependencies.

При необходимости используй:

- readelf;
- objdump;
- patchelf.

Например:

официальный:

/usr/bin/example

после адаптации:

/nex/packages/example/1.0/bin/example

Если нужно, исправляй:

- ELF interpreter;
- RPATH;
- RUNPATH;
- library references.

---

# 19. SCRIPTS

Если программа содержит:

#!/usr/bin/env python3

или:

#!/bin/bash

проверь наличие соответствующего runtime.

Если используется жёсткий путь:

/usr/bin/python3

адаптируй его к Nexa Core или обеспечь корректную системную интеграцию.

---

# 20. DESKTOP FILES

Для GUI-программ обрабатывай:

*.desktop

Например:

Exec=/usr/bin/example

должно корректно запускать:

/nex/packages/example/<version>/bin/example

Сам desktop-файл остаётся внутри package:

/nex/packages/example/<version>/share/applications/

---

# 21. SYMLINKS

Корректно обрабатывай symlinks.

Не допускай ссылок на:

- build directory;
- temporary directory;
- cache;
- удалённые пакеты.

---

# 22. BINARY VALIDATION

После адаптации проверяй:

- архитектуру;
- ELF validity;
- interpreter;
- DT_NEEDED;
- libraries;
- RPATH/RUNPATH;
- symlinks;
- executability;
- package paths.

Если автоматическая адаптация небезопасна — установка должна остановиться с понятной ошибкой.

Не устанавливай потенциально повреждённый бинарник.

---

# 23. ФОРМАТЫ BINARY

Постепенно поддержи:

- tar;
- tar.gz;
- tar.xz;
- tar.zst;
- zip;
- обычные ELF binaries;
- AppImage;
- Linux application bundles.

Каждый формат реализуй отдельным модулем.

---

# 24. OFFICIAL SOURCES

Для binary packages предпочитай официальные источники проектов.

Не используй неизвестные repack'и, если доступен официальный release.

Recipe обязательно должен содержать checksum.

---

# 25. PACKAGE DATABASE

Используй простую SQLite database:

/nex/db/packages.db

Храни:

- package name;
- version;
- description;
- dependencies;
- build dependencies;
- files;
- source;
- checksum;
- installation state.

Не усложняй database.

---

# 26. BUILD SANDBOX

Сборка должна происходить в отдельной директории:

/nex/build/<package>/

Package build не должен случайно записывать файлы в реальную систему.

После сборки:

build
→ staging
→ validation
→ /nex/packages/<name>/<version>/

---

# 27. УДАЛЕНИЕ ПАКЕТОВ

При:

nex remove nano

удали:

/nex/packages/nano/<version>/

и связанные с ним системные ссылки.

Не удаляй файлы других пакетов.

Перед удалением проверяй reverse dependencies.

---

# 28. LIVE CD

Первый результат проекта:

**Nexa Core Live CD**

Пока БЕЗ установщика.

Только CLI/TTY.

ISO:

Nexa-Core-<version>-x86_64.iso

Должно поддерживаться UEFI.

BIOS желательно добавить позже.

---

# 29. LIVE CD SOFTWARE

Минимальный набор:

- Linux kernel;
- systemd;
- bash;
- coreutils;
- util-linux;
- mount;
- umount;
- lsblk;
- blkid;
- fdisk;
- cfdisk;
- dmesg;
- ip;
- ping;
- passwd;
- login;
- nano;
- grep;
- sed;
- awk;
- tar;
- gzip;
- xz;
- find;
- ps;
- kill;
- chmod;
- chown;
- cp;
- mv;
- rm;
- mkdir;
- ln;
- cat;
- less;
- df;
- du;
- journalctl;
- systemctl;
- fsck;
- e2fsprogs;
- swap tools;
- ssh желательно;
- curl или wget;
- `nex`.

---

# 30. RECOVERY

Live CD должен позволять:

- посмотреть диски;
- создавать/изменять разделы;
- монтировать filesystem;
- проверять filesystem;
- менять пароль;
- просматривать journal;
- использовать `nex`;
- проверять пакеты;
- переустанавливать пакеты;
- удалять повреждённые пакеты;
- восстанавливать системные ссылки;
- входить в установленную систему через recovery shell.

Команда:

nex repair --root /mnt

---

# 31. BOOT

Первоначально разрешается использовать GRUB.

Boot chain:

Bootloader
→ Linux kernel
→ initramfs
→ root filesystem
→ systemd
→ TTY

---

# 32. INITRAMFS

Создай минимальный initramfs tooling.

Он должен:

- подготовить `/dev`;
- `/proc`;
- `/sys`;
- найти root filesystem;
- смонтировать root;
- передать управление systemd.

Не создавай огромную систему уровня dracut.

---

# 33. ПЕРВЫЙ ТЕСТОВЫЙ ПАКЕТ

Создай:

hello-nex

После:

nex install hello-nex

должно появиться:

/nex/packages/hello-nex/<version>/bin/hello

И:

hello

должно вывести:

Hello from Nexa Core!

После:

nex remove hello-nex

пакет должен исчезнуть.

---

# 34. ПРИМЕРЫ РЕАЛЬНЫХ NEXBUILD RECIPES

Это обязательная часть проекта.

Не ограничивайся `hello-nex`.

Создай каталог:

examples/packages/

и напиши реальные рабочие `.nxb` recipes.

---

# 35. БАЗОВЫЕ ПРОГРАММЫ

Создай recipes минимум для:

- bash;
- coreutils;
- grep;
- sed;
- awk;
- findutils;
- nano;
- vim;
- less;
- tar;
- gzip;
- xz;
- zstd;
- curl;
- wget;
- git;
- openssh;
- sudo;
- which;
- file;
- pciutils;
- usbutils;
- procps;
- iproute2;
- iputils;
- e2fsprogs;
- util-linux.

Для каждого используй официальные upstream sources.

---

# 36. ПОПУЛЯРНЫЕ ПРОГРАММЫ

Добавь примеры для популярных приложений, где это технически возможно:

- Firefox;
- Chromium;
- VLC;
- mpv;
- Git;
- Python;
- Node.js;
- CMake;
- Meson;
- GCC;
- Neovim;
- fastfetch;
- htop;
- btop;
- NetworkManager;
- PipeWire;
- WirePlumber.

Для программ, которые распространяются преимущественно бинарными релизами, покажи binary recipe.

Для проектов с хорошими исходниками покажи source recipe.

---

# 37. DESKTOP ENVIRONMENTS

Создай отдельные примеры установки популярных DE.

Минимально:

## KDE Plasma

Recipes/метапакет для:

- Plasma;
- KDE Frameworks;
- KDE applications;
- SDDM;
- необходимого Wayland/X11 stack.

Например:

examples/desktop/kde-plasma.nxb

Он должен показывать, как NexBuild может объединять несколько зависимостей.

## GNOME

Создай аналогичный пример:

examples/desktop/gnome.nxb

С зависимостями:

- GNOME Shell;
- GTK;
- GDM или соответствующая login manager integration;
- необходимые сервисы.

## XFCE

Создай:

examples/desktop/xfce.nxb

---

# 38. WINDOW MANAGERS

Добавь примеры для популярных WM/compositor:

- Hyprland;
- Sway;
- i3;
- Openbox.

Например:

examples/wm/hyprland.nxb
examples/wm/sway.nxb
examples/wm/i3.nxb
examples/wm/openbox.nxb

Рецепты должны показывать реальные зависимости и build steps.

---

# 39. DISPLAY MANAGERS

Добавь примеры:

- SDDM;
- GDM;
- LightDM.

Особенно хорошо документируй SDDM.

Например:

examples/display-managers/sddm.nxb

---

# 40. SDDM THEMES / SKINS

Nexa Core должна поддерживать установку тем SDDM как обычных пакетов.

Например:

/nex/packages/sddm-theme-example/<version>/

с:

share/sddm/themes/<theme-name>/

Создай несколько example recipes для реально существующих open-source SDDM themes.

Например:

examples/sddm-themes/

Каждый theme recipe должен:

1. скачать официальный source;
2. проверить checksum;
3. установить theme в package directory;
4. выполнить необходимую Nexa Core integration;
5. сделать theme доступной для SDDM.

Если SDDM ожидает:

/usr/share/sddm/themes/

не копируй theme напрямую туда.

Используй системную интеграцию Nexa Core.

---

# 41. SYSTEMD SERVICES

Создай examples/services/

и recipes для популярных служб:

- NetworkManager;
- sshd;
- bluetooth;
- PipeWire;
- WirePlumber;
- cron/anacron alternative;
- cups;
- avahi;
- docker/container runtime как пример optional service.

Для каждой службы покажи:

- package;
- unit files;
- installation;
- enable;
- start;
- dependencies.

Например:

nex install openssh
systemctl enable sshd
systemctl start sshd

---

# 42. META PACKAGES

NexBuild должен поддерживать простой способ создавать meta packages.

Например:

kde-plasma

не обязательно содержит собственные файлы.

Он может просто зависеть от:

plasma-desktop
plasma-workspace
kwin
sddm
...

То же самое:

gnome
xfce
hyprland-desktop

Это должно быть простым механизмом.

---

# 43. ДОКУМЕНТАЦИЯ ПРИМЕРОВ

Создай:

docs/examples.md

Опиши:

- как установить базовую систему;
- как установить KDE Plasma;
- как установить GNOME;
- как установить XFCE;
- как установить Hyprland;
- как установить Sway;
- как установить i3;
- как установить SDDM;
- как установить SDDM theme;
- как включить NetworkManager;
- как включить SSH;
- как включить PipeWire;
- как установить Firefox;
- как установить Chromium;
- как установить VLC;
- как установить Python;
- как установить Git.

---

# 44. REPOSITORY

Repository Nexa Core должен хранить прежде всего recipes, а не бинарные пакеты.

Например:

packages/
├── system/
├── base/
├── desktop/
├── wm/
├── applications/
├── services/
└── themes/

Binary sources могут находиться на официальных серверах upstream-проектов.

---

# 45. BUILD SYSTEM

Создай единый build tool.

Например:

./tools/build bootstrap
./tools/build system
./tools/build package
./tools/build live
./tools/build iso

Он должен позволять разработчику собрать Live CD одной командой.

---

# 46. QEMU

Создай:

./tools/run-qemu.sh

Он должен запускать ISO в QEMU.

Минимально:

- x86_64;
- 2 CPU;
- 2 GB RAM;
- ISO;
- serial/TTY.

Создай smoke tests:

- kernel boots;
- systemd starts;
- TTY starts;
- login works;
- `nex --version`;
- `/nex` exists;
- package database works;
- `hello-nex` installs;
- `hello` запускается;
- package удаляется.

---

# 47. ДОКУМЕНТАЦИЯ

Создай:

docs/
├── architecture.md
├── bootstrap.md
├── package-manager.md
├── nexbuild.md
├── binary-adapter.md
├── package-format.md
├── filesystem.md
├── livecd.md
├── recovery.md
├── boot.md
├── examples.md
├── desktop-environments.md
├── window-managers.md
├── services.md
├── sddm-themes.md
├── development.md
└── contributing.md

Особенно подробно напиши:

`docs/nexbuild.md`

и:

`docs/binary-adapter.md`

---

# 48. NEXBUILD DOCUMENTATION

Документация должна содержать:

- полный синтаксис;
- package;
- version;
- metadata;
- source;
- binary;
- checksum;
- depends;
- makedepends;
- provides;
- conflicts;
- prepare;
- build;
- check;
- install;
- patch;
- service;
- environment;
- переменные;
- meta packages;
- примеры;
- ошибки;
- правила безопасности.

---

# 49. ПОРЯДОК РЕАЛИЗАЦИИ

Не пиши весь проект одним огромным коммитом.

Работай последовательно:

1. структура проекта;
2. bootstrap toolchain;
3. root filesystem;
4. systemd;
5. boot;
6. initramfs;
7. TTY;
8. `/nex`;
9. NexBuild parser;
10. package manager;
11. package database;
12. source packages;
13. binary adapter;
14. binary packages;
15. hello-nex;
16. базовые системные packages;
17. Live CD;
18. QEMU;
19. реальные recipes;
20. DE/WM recipes;
21. services;
22. SDDM themes;
23. документация;
24. тестирование.

Установщик системы делать только после завершения рабочего Live CD.

---

# 50. ВАЖНЫЕ ОГРАНИЧЕНИЯ

НЕ добавляй:

- generations;
- snapshots;
- rollback system;
- сложные profiles;
- immutable generations;
- бинарный repository как обязательную основу;
- GUI installer на первом этапе;
- desktop environment в базовый Live CD;
- собственный kernel;
- собственную libc.

Не усложняй систему ради архитектуры.

Главная идея Nexa Core:

SOURCE RECIPE
       ↓
    BUILD
       ↓
VALIDATE
       ↓
/nex/packages/<name>/<version>/

или:

OFFICIAL BINARY
       ↓
     VERIFY
       ↓
     EXTRACT
       ↓
     ADAPT
       ↓
    VALIDATE
       ↓
/nex/packages/<name>/<version>/

Оба варианта должны приводить к одному типу установленного Nexa Core package.

---

# 51. КОНЕЧНЫЙ РЕЗУЛЬТАТ ПЕРВОГО ЭТАПА

Команда:

./tools/build iso

должна создавать:

Nexa-Core-<version>-x86_64.iso

ISO должно загружаться в QEMU.

После загрузки пользователь получает TTY:

Nexa Core
nexa login:

И может использовать:

nex
nano
passwd
mount
lsblk
fdisk
systemctl
journalctl
bash

Package manager должен уметь:

nex install hello-nex

после чего:

hello

выводит:

Hello from Nexa Core!

После:

nex remove hello-nex

пакет полностью удаляется.

Также в репозитории должны уже находиться реальные примеры NexBuild для:

- базовых Linux-программ;
- Firefox;
- Chromium;
- VLC;
- Python;
- Git;
- KDE Plasma;
- GNOME;
- XFCE;
- Hyprland;
- Sway;
- i3;
- Openbox;
- SDDM;
- нескольких SDDM themes;
- NetworkManager;
- OpenSSH;
- PipeWire;
- WirePlumber;
- других популярных systemd services.

Начни с проектирования структуры репозитория и bootstrap-системы.
Не создавай установщик до того, как Live CD будет реально загружаться и работать в QEMU.
